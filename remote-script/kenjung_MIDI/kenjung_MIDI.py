import Live
from _Framework.ControlSurface import ControlSurface

MANUFACTURER_ID = 0x7D  # 非商用 SysEx ID
SYSEX_TRACK_COLOR = 0x01

MAX_TRACKS = 12  # フェーダー最大本数（NUM_FADERS と合わせる）


def _color_to_7bit_rgb(color_int):
    """Ableton の track.color（24bit int）を 7bit RGB に変換する"""
    r = ((color_int >> 16) & 0xFF) >> 1
    g = ((color_int >> 8)  & 0xFF) >> 1
    b = ( color_int        & 0xFF) >> 1
    return r, g, b


class KenjungMIDI(ControlSurface):

    def __init__(self, c_instance):
        super().__init__(c_instance)
        self.log_message('kenjung MIDI: init start')
        self._track_listeners = {}
        with self.component_guard():
            self._setup_track_listeners()
            self.song().add_tracks_listener(self._on_tracks_changed)
        self.log_message('kenjung MIDI: init done')

    def disconnect(self):
        self._remove_track_listeners()
        self.song().remove_tracks_listener(self._on_tracks_changed)
        super().disconnect()

    # ------------------------------------------------------------------

    def _setup_track_listeners(self):
        self._remove_track_listeners()
        for i, track in enumerate(self.song().tracks[:MAX_TRACKS]):
            def make_listener(idx, t):
                def listener():
                    self._send_track_color(idx, t.color)
                return listener
            fn = make_listener(i, track)
            track.add_color_listener(fn)
            self._track_listeners[track] = fn
            self._send_track_color(i, track.color)

    def _remove_track_listeners(self):
        for track, fn in self._track_listeners.items():
            if track.color_has_listener(fn):
                track.remove_color_listener(fn)
        self._track_listeners.clear()

    def _on_tracks_changed(self):
        self._setup_track_listeners()

    def _send_track_color(self, index, color_int):
        r, g, b = _color_to_7bit_rgb(color_int)
        sysex = (0xF0, MANUFACTURER_ID, SYSEX_TRACK_COLOR, index, r, g, b, 0xF7)
        self.log_message('send color track[{}] R={} G={} B={}'.format(index, r, g, b))
        self._send_midi(sysex)
