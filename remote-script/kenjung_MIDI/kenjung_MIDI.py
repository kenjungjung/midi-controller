# Ableton Live Remote Script — ESP32 MIDIコントローラー用
#
# このスクリプトの仕事：
#   1. トラックカラーをSysExでESP32に送信し、LEDカラーをリアルタイム同期する。
#   2. トラックの出力レベル（VUメーター値）を一定間隔でポーリングしてESP32に送る。
#      → フェーダー位置ではなく「実際に鳴っている音の大きさ」を送る。
#
# 配置先：
#   C:\Users\kengo\Documents\Ableton\User Library\Remote Scripts\kenjung_MIDI\

import Live
from _Framework.ControlSurface import ControlSurface


# ── 定数 ──────────────────────────────────────────────────────────────────────

# SysEx メーカーID。0x7D は「開発用フリーID」として規格で予約されている。
MANUFACTURER_ID = 0x7D

# SysEx コマンドID。どんな命令かを識別するための番号。
SYSEX_TRACK_COLOR  = 0x01  # Ableton → ESP32: トラックカラーを送る
SYSEX_REFRESH_REQ  = 0x02  # ESP32 → Ableton: 全データを再送してほしい
SYSEX_TRACK_METER  = 0x03  # Ableton → ESP32: トラック出力レベルを送る

# 対応するトラック数（＝LEDの個数）。これを超えたトラックは無視する。
MAX_TRACKS = 12

# メーターポーリング間隔（Abletonのスケジューラ単位）。
# schedule_message の単位は約100ms なので、2 = 約200ms ≈ 5fps。
# 速くするほど滑らかになるが、MIDI帯域を消費する。
METER_POLL_INTERVAL = 2


# ── ユーティリティ ─────────────────────────────────────────────────────────────

def _color_to_7bit_rgb(color_int):
    """
    Ableton の色 (0xRRGGBB) を MIDI で送れる 7bit 値 (0–127) に変換する。

    MIDI のデータバイトは最上位ビットが常に 0 でなければならない（8bit目が1だと
    ステータスバイトと区別できなくなる）。そのため 8bit (0–255) を 1bit 右シフト
    して 7bit (0–127) に収める。色の精度は半分になるが、LEDには十分。
    """
    r = ((color_int >> 16) & 0xFF) >> 1  # 上位8bit = 赤
    g = ((color_int >>  8) & 0xFF) >> 1  # 中位8bit = 緑
    b = ( color_int        & 0xFF) >> 1  # 下位8bit = 青
    return r, g, b


def _meter_to_7bit(level_float):
    """
    Ableton の出力レベル (0.0–1.0) を MIDI 7bit 値 (0–127) に変換する。

    output_meter_level は 0.0（無音）〜 1.0（クリップ直前）の float。
    clamp で範囲外を弾く。
    """
    return max(0, min(127, int(level_float * 127)))


# ── メインクラス ───────────────────────────────────────────────────────────────

class KenjungMIDI(ControlSurface):
    """
    Ableton が Remote Script として読み込むクラス。
    ControlSurface を継承することで、Live の内部 API にアクセスできる。
    """

    def _log(self, msg):
        """Ableton の Log.txt にデバッグメッセージを出力する。"""
        self.log_message(f"[KJ] {msg}")

    # ── ライフサイクル ────────────────────────────────────────────────────────

    def __init__(self, c_instance):
        """
        Ableton 起動時・スクリプト選択時に1回だけ呼ばれる初期化処理。

        c_instance は Ableton が渡してくる内部オブジェクト。
        ControlSurface.__init__ に渡すだけでよく、直接触る必要はない。
        """
        super().__init__(c_instance)
        self._log("init")

        # (track, color_fn) のペアを保持するリスト。
        # ここに入れておかないと Python の GC（メモリ管理）が勝手に削除してしまい、
        # コールバックが届かなくなる。
        self._track_listeners = []

        # component_guard() は Ableton が提供する安全な初期化ブロック。
        # この中に書くことで、例外が起きてもスクリプト全体がクラッシュしない。
        with self.component_guard():
            # トラックの追加・削除・並び替えを監視する。
            self.song().add_visible_tracks_listener(self._on_tracks_changed)

            # 各トラックのカラー変更リスナーを登録する。
            self._build_track_listeners()

            # Ableton は起動直後に状態が不安定なため、数フレーム後に初回同期する。
            # 0 を指定すると即時実行になりクラッシュする場合があるので 10 を使う。
            self.schedule_message(10, self._sync_all_tracks)

            # メーターのポーリングループを開始する。
            self.schedule_message(10, self._poll_meters)

    def disconnect(self):
        """
        スクリプトが切り離されるとき（Ableton 終了・スクリプト変更）に呼ばれる。
        登録したリスナーを必ず解除しないとメモリリークする。
        ポーリングループは schedule_message ベースなので自動的に止まる。
        """
        self._log("disconnect")
        self._clear_track_listeners()
        if self.song().visible_tracks_has_listener(self._on_tracks_changed):
            self.song().remove_visible_tracks_listener(self._on_tracks_changed)
        super().disconnect()

    # ── イベントハンドラ ──────────────────────────────────────────────────────

    def _on_tracks_changed(self):
        """
        トラックの追加・削除・並び替えが起きたときに Ableton から呼ばれる。
        リスナーの対象が変わるので、再構築してから全カラーを送り直す。
        """
        self._log("tracks changed")
        self._sync_all_tracks()
        self._build_track_listeners()

    def receive_midi(self, midi_bytes):
        """
        ESP32 から MIDI を受信したときに Ableton が呼ぶメソッド。
        ESP32 が起動・リセットしたときに「データを全部送り直して」と要求してくる。

        SysEx フォーマット: F0 <MANUFACTURER_ID> <CMD> ... F7
        """
        if len(midi_bytes) >= 4 and midi_bytes[0] == 0xF0 and midi_bytes[-1] == 0xF7:
            if midi_bytes[1] == MANUFACTURER_ID and midi_bytes[2] == SYSEX_REFRESH_REQ:
                self._log("refresh request")
                self._build_track_listeners()
                self.schedule_message(10, self._sync_all_tracks)

    # ── 内部処理 ─────────────────────────────────────────────────────────────

    def _get_tracks(self):
        """現在 Ableton の画面に表示されているトラック一覧を返す。"""
        return list(self.song().visible_tracks)

    def _sync_all_tracks(self):
        """
        MAX_TRACKS 分のカラーを ESP32 に一括送信する。
        トラックが存在しない枠は黒 (0x000000) を送って消灯させる。
        """
        self._log("sync all tracks")
        tracks = self._get_tracks()
        for i in range(MAX_TRACKS):
            color = tracks[i].color if i < len(tracks) else 0x000000
            self._send_track_color(i, color)

    def _poll_meters(self):
        """
        全トラックの出力レベル (output_meter_level) を読み取って ESP32 に送る。

        output_meter_level はリスナーが存在しないためポーリングが必要。
        送信後に自分自身を再スケジュールすることで一定間隔のループを作る。
        disconnect() 後は schedule_message が呼ばれないのでループは自然に止まる。
        """
        tracks = self._get_tracks()
        for i in range(MAX_TRACKS):
            level = tracks[i].output_meter_level if i < len(tracks) else 0.0
            self._send_track_meter(i, level)

        # 次のポーリングをスケジュール（再帰ではなくキューに積む）
        self.schedule_message(METER_POLL_INTERVAL, self._poll_meters)

    def _get_track_index(self, track):
        """
        トラックの現在のインデックスを返す。対象外なら None を返す。
        トラック削除直後にも呼ばれることがあるので、リストにいるか確認してから返す。
        """
        tracks = self._get_tracks()
        if track not in tracks:
            return None  # すでに削除されたトラック
        idx = tracks.index(track)
        return idx if idx < MAX_TRACKS else None

    def _handle_track_color_change(self, track):
        """特定のトラックの色が変わったときに呼ばれる。"""
        idx = self._get_track_index(track)
        if idx is not None:
            self._send_track_color(idx, track.color)

    def _build_track_listeners(self):
        """
        各トラックに「色が変わったら通知して」というリスナーを登録する。
        トラック構成が変わるたびに呼び直す必要がある（古いリスナーを一度消してから登録）。
        """
        self._log("build listeners")
        self._clear_track_listeners()

        for track in self._get_tracks()[:MAX_TRACKS]:
            # Python のループ変数は参照渡しなので、そのままクロージャに入れると
            # 最後の track だけが全リスナーで参照される。
            # make_listener() で囲んで引数として渡すことで、各ループの値を固定する。
            def make_listener(t):
                def color_listener():
                    self._handle_track_color_change(t)
                return color_listener

            fn = make_listener(track)
            track.add_color_listener(fn)
            self._track_listeners.append((track, fn))

    def _clear_track_listeners(self):
        """登録済みのリスナーをすべて解除してリストを空にする。"""
        for track, fn in self._track_listeners:
            if track.color_has_listener(fn):
                track.remove_color_listener(fn)
        self._track_listeners.clear()

    # ── MIDI 送信 ─────────────────────────────────────────────────────────────

    def _send_track_color(self, index, color_int):
        """
        1トラック分のカラーを SysEx で ESP32 に送る。

        SysEx フォーマット:
          F0  : SysEx 開始
          7D  : MANUFACTURER_ID
          01  : SYSEX_TRACK_COLOR コマンド
          idx : トラック番号 (0–11)
          r   : 赤 (0–127)
          g   : 緑 (0–127)
          b   : 青 (0–127)
          F7  : SysEx 終了
        """
        r, g, b = _color_to_7bit_rgb(color_int)
        sysex = (0xF0, MANUFACTURER_ID, SYSEX_TRACK_COLOR, index, r, g, b, 0xF7)
        self._send_midi(sysex)

    def _send_track_meter(self, index, level_float):
        """
        1トラック分の出力レベルを SysEx で ESP32 に送る。

        SysEx フォーマット:
          F0  : SysEx 開始
          7D  : MANUFACTURER_ID
          03  : SYSEX_TRACK_METER コマンド
          idx : トラック番号 (0–11)
          lvl : 出力レベル (0–127)
          F7  : SysEx 終了
        """
        lvl = _meter_to_7bit(level_float)
        sysex = (0xF0, MANUFACTURER_ID, SYSEX_TRACK_METER, index, lvl, 0xF7)
        self._send_midi(sysex)

    def refresh_state(self):
        """
        外部から手動で再同期するためのエントリポイント。
        Ableton の UI から呼ばれることがある（将来用）。
        """
        self._log("manual refresh")
        self._build_track_listeners()
        self.schedule_message(10, self._sync_all_tracks)
