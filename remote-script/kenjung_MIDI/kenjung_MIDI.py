# ============================================================
# ▼ このスクリプトの役割（めちゃ重要）
# ------------------------------------------------------------
# Ableton Live のトラックの色を取得して、
# ESP32（自作MIDIコントローラ）に送るプログラム
#
# つまり：
# Ableton → Pythonスクリプト → ESP32
#
# トラックの色が変わったらリアルタイムで送信される
# ============================================================


# ============================================================
# ▼ 事前準備
# ------------------------------------------------------------
# このフォルダを以下に配置すること：
# C:\Users\kengo\Documents\Ableton\User Library\Remote Scripts
#
# するとAbletonの「MIDI Remote Scripts」に出てくる
# ============================================================


import Live
from _Framework.ControlSurface import ControlSurface


# ============================================================
# ▼ 設定（あとでハードと合わせる）
# ============================================================

# SysEx（特殊なMIDIメッセージ）で使うメーカーID
# 0x7Dは「開発用で自由に使っていいID」
MANUFACTURER_ID = 0x7D

# 「トラックカラーを送る」という命令ID
SYSEX_TRACK_COLOR = 0x01

# 最大トラック数（LEDの数とかに合わせる）
MAX_TRACKS = 12


# ============================================================
# ▼ ユーティリティ関数
# ============================================================

def _color_to_7bit_rgb(color_int):
    """
    Abletonの色（0xRRGGBB）をRGBに分解して
    MIDIで送れる0〜127に変換する

    なぜ？
    → MIDIは7bit（0〜127）しか扱えないため
    """

    # 赤成分を取り出す（16bit右シフト）
    r = ((color_int >> 16) & 0xFF) >> 1

    # 緑
    g = ((color_int >> 8)  & 0xFF) >> 1

    # 青
    b = ( color_int        & 0xFF) >> 1

    return r, g, b


# ============================================================
# ▼ メインクラス（Abletonが呼び出す）
# ============================================================

class KenjungMIDI(ControlSurface):

    # --------------------------------------------------------
    # ▼ ログ出力
    # --------------------------------------------------------

    def _log(self, msg):
        """AbletonのLog.txtに出る（デバッグ用）"""
        self.log_message(f"[KJ] {msg}")


    # --------------------------------------------------------
    # ▼ 初期化（Ableton起動時に1回だけ呼ばれる）
    # --------------------------------------------------------

    def __init__(self, c_instance):
        super().__init__(c_instance)

        self._log("init")

        # ----------------------------------------------------
        # ▼ listenerとは？
        # ----------------------------------------------------
        # 「何か変化が起きたら自動で呼ばれる関数」
        #
        # 例：
        # ・トラック追加された
        # ・色が変わった
        #
        # → いちいち監視しなくていい（イベント駆動）
        # ----------------------------------------------------

        # listenerを保持するリスト
        # （保持しないとPythonが勝手に消す = 超重要）
        self._track_listeners = []

        # Abletonの安全な初期化ブロック
        with self.component_guard():

            # トラック構成の変化を監視
            self.song().add_visible_tracks_listener(self._on_tracks_changed)

            # 各トラックの色変更を監視するlistenerを作る
            self._build_track_listeners()

            # ------------------------------------------------
            # ▼ なぜ遅延するの？
            # ------------------------------------------------
            # Abletonは起動直後だと状態がまだ不安定
            #
            # → すぐ実行するとバグることがある
            #
            # → 少し待ってから実行する（これ超重要）
            # ------------------------------------------------
            self.schedule_message(10, self._sync_all_tracks)


    # --------------------------------------------------------
    # ▼ 終了処理
    # --------------------------------------------------------

    def disconnect(self):
        self._log("disconnect")

        # listenerを全部削除（メモリリーク防止）
        self._clear_track_listeners()

        # トラック監視も解除
        if self.song().visible_tracks_has_listener(self._on_tracks_changed):
            self.song().remove_visible_tracks_listener(self._on_tracks_changed)

        super().disconnect()


    # --------------------------------------------------------
    # ▼ トラック構成が変わったとき
    # --------------------------------------------------------

    def _on_tracks_changed(self):
        """
        呼ばれるタイミング：
        ・トラック追加
        ・削除
        ・並び替え
        """
        self._log("tracks changed")

        # まず今の状態を送る
        self._sync_all_tracks()

        # listenerを作り直す（これ重要）
        self._build_track_listeners()


    # --------------------------------------------------------
    # ▼ MIDI受信（ESP32 → Ableton）
    # --------------------------------------------------------

    def receive_midi(self, midi_bytes):
        """
        ESP32から「リフレッシュして」と言われた時に動く
        """

        # SysExかどうかチェック
        if len(midi_bytes) >= 4 and midi_bytes[0] == 0xF0 and midi_bytes[-1] == 0xF7:

            # 自分宛か確認
            if midi_bytes[1] == MANUFACTURER_ID:

                # 0x02 = リフレッシュ要求
                if midi_bytes[2] == 0x02:
                    self._log("refresh request")

                    # listenerを再構築
                    self._build_track_listeners()

                    # 少し待ってから同期
                    self.schedule_message(10, self._sync_all_tracks)


    # --------------------------------------------------------
    # ▼ 内部処理
    # --------------------------------------------------------

    def _get_tracks(self):
        """現在表示されているトラック一覧"""
        return list(self.song().visible_tracks)


    def _sync_all_tracks(self):
        """
        全トラックの色をESP32に送る
        """
        self._log("sync all tracks")

        tracks = self._get_tracks()

        # MAX_TRACKS分ループ
        for i in range(MAX_TRACKS):

            if i < len(tracks):
                # トラックがある
                self._send_track_color(i, tracks[i].color)
            else:
                # トラックがない → LED消す
                self._send_track_color(i, 0x000000)


    def _handle_track_color_change(self, track):
        """
        トラックの色が変わったときに呼ばれる
        """

        tracks = self._get_tracks()

        # すでに削除された場合は無視
        if track not in tracks:
            return

        idx = tracks.index(track)

        if idx < MAX_TRACKS:
            self._send_track_color(idx, track.color)


    def _build_track_listeners(self):
        """
        各トラックの色変更を監視する
        """

        self._log("build listeners")

        # 一度全部削除
        self._clear_track_listeners()

        tracks = self._get_tracks()[:MAX_TRACKS]

        for track in tracks:

            # ------------------------------------------------
            # ▼ 超重要：クロージャ問題
            # ------------------------------------------------
            # Pythonはループ変数をそのまま使うとバグる
            #
            # → 関数で包んで固定する必要がある
            # ------------------------------------------------
            def make_listener(t):
                def listener():
                    self._handle_track_color_change(t)
                return listener

            fn = make_listener(track)

            track.add_color_listener(fn)

            # 参照保持（しないと消える）
            self._track_listeners.append((track, fn))


    def _clear_track_listeners(self):
        """listenerを全削除"""

        for track, fn in self._track_listeners:
            if track.color_has_listener(fn):
                track.remove_color_listener(fn)

        self._track_listeners.clear()


    # --------------------------------------------------------
    # ▼ MIDI送信（Ableton → ESP32）
    # --------------------------------------------------------

    def _send_track_color(self, index, color_int):
        """
        トラックの色をESP32に送る
        """

        r, g, b = _color_to_7bit_rgb(color_int)

        # SysExフォーマット
        # F0:開始 / F7:終了
        sysex = (0xF0, MANUFACTURER_ID, SYSEX_TRACK_COLOR, index, r, g, b, 0xF7)

        self._log(f"send idx={index} r={r} g={g} b={b}")

        self._send_midi(sysex)


    # --------------------------------------------------------
    # ▼ 外部から呼べる関数
    # --------------------------------------------------------

    def refresh_state(self):
        """
        手動で再同期したいときに使う
        """

        self._log("manual refresh")

        self._build_track_listeners()

        # 即時実行は危険 → 遅延
        self.schedule_message(10, self._sync_all_tracks)