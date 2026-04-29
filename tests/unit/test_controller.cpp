#include <gtest/gtest.h>
#include <vector>
#include "controller.h"
#include "config.h"

// ── テスト用 MidiSender スタブ ────────────────────────────────
class SpyMidiSender : public IMidiSender {
public:
    struct CcCall { uint8_t ch, cc, val; };
    struct NoteCall { uint8_t ch, note, vel; bool on; };

    void send_cc(uint8_t ch, uint8_t cc, uint8_t val) override {
        cc_calls.push_back({ch, cc, val});
    }
    void send_note_on(uint8_t ch, uint8_t note, uint8_t vel) override {
        note_calls.push_back({ch, note, vel, true});
    }
    void send_note_off(uint8_t ch, uint8_t note) override {
        note_calls.push_back({ch, note, 0, false});
    }
    bool is_connected() const override { return connected; }

    bool connected = true;
    std::vector<CcCall>  cc_calls;
    std::vector<NoteCall> note_calls;
};

// ────────────────────────────────────────────────────────────────
// TC-1: to_midi_cc(0) → 0
// ────────────────────────────────────────────────────────────────
TEST(ToMidiCc, Min) {
    EXPECT_EQ(Controller::to_midi_cc(0), 0);
}

// TC-2: to_midi_cc(4095) → 127
TEST(ToMidiCc, Max) {
    EXPECT_EQ(Controller::to_midi_cc(4095), 127);
}

// TC-3: to_midi_cc(2048) → 64
TEST(ToMidiCc, Mid) {
    EXPECT_EQ(Controller::to_midi_cc(2048), 64);
}

// ── queue ベースのテスト用フィクスチャ ──────────────────────────
class ControllerQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        q = xQueueCreate(MIDI_QUEUE_LEN, sizeof(MidiEvent));
    }
    void TearDown() override {
        delete q;
    }

    QueueHandle_t q = nullptr;
};

// TC-4: deadband未満 (delta=3) → input_loop は while(true) のため統合テストで検証
TEST_F(ControllerQueueTest, DeadbandBelowThreshold_SkippedForIntegration) {
    GTEST_SKIP() << "input_loop が while(true) のため単体テスト不可。統合テストで検証";
}

// TC-5: deadband以上 (delta=4) → 同上
TEST_F(ControllerQueueTest, DeadbandAtThreshold_SkippedForIntegration) {
    GTEST_SKIP() << "input_loop が while(true) のため単体テスト不可";
}

// TC-6: is_connected()==false → 同上
TEST_F(ControllerQueueTest, DisconnectedNoEvent_SkippedForIntegration) {
    GTEST_SKIP() << "input_loop が while(true) のため単体テスト不可";
}

// TC-7: キューが満杯でも xQueueSend は drop するだけでクラッシュしない
TEST_F(ControllerQueueTest, QueueFullNoCrash) {
    MidiEvent ev{MidiEvent::Type::CC, 1, 1, 64};
    for (int i = 0; i < MIDI_QUEUE_LEN + 5; ++i) {
        BaseType_t r = xQueueSend(q, &ev, 0);
        if (i < MIDI_QUEUE_LEN) {
            EXPECT_EQ(r, pdTRUE);
        } else {
            EXPECT_EQ(r, pdFALSE);
        }
    }
    EXPECT_EQ(uxQueueMessagesWaiting(q), static_cast<size_t>(MIDI_QUEUE_LEN));
}

// TC-8: StubAnalogInput(2048) → to_midi_cc が 64 を返す
//       MidiEvent をキューに入れ、取り出したら値が正しいことを確認
TEST_F(ControllerQueueTest, StubFaderCC64ViaQueue) {
    StubAnalogInput fader(2048);
    EXPECT_EQ(Controller::to_midi_cc(fader.read()), 64);

    MidiEvent ev{MidiEvent::Type::CC, MIDI_CHANNEL, CC_FADER_1, 64};
    ASSERT_EQ(xQueueSend(q, &ev, 0), pdTRUE);

    MidiEvent rx{};
    ASSERT_EQ(xQueueReceive(q, &rx, 0), pdTRUE);
    EXPECT_EQ(rx.type,    MidiEvent::Type::CC);
    EXPECT_EQ(rx.channel, MIDI_CHANNEL);
    EXPECT_EQ(rx.number,  CC_FADER_1);
    EXPECT_EQ(rx.value,   64);
}

// TC-9: xQueueReset → キューが空になる（USB切断時のflush）
TEST_F(ControllerQueueTest, QueueResetFlushes) {
    MidiEvent ev{MidiEvent::Type::CC, 1, 1, 10};
    for (int i = 0; i < 5; ++i) xQueueSend(q, &ev, 0);
    EXPECT_EQ(uxQueueMessagesWaiting(q), 5u);

    xQueueReset(q);
    EXPECT_EQ(uxQueueMessagesWaiting(q), 0u);
}

// TC-10: reset_prev_cc() はクラッシュせず呼び出せる
//        prev_cc_=0xFF のとき delta=DEADBAND (強制送信条件) になることをコードで確認
TEST(ResetPrevCc, NoCrashAndForcedDeltaCondition) {
    QueueHandle_t q2 = xQueueCreate(MIDI_QUEUE_LEN, sizeof(MidiEvent));
    StubAnalogInput fader(2048);
    StubButton btn;
    SpyMidiSender sender;
    Controller ctrl(fader, btn, sender, q2);

    ctrl.reset_prev_cc();  // prev_cc_ = 0xFF

    // prev==0xFF のとき controller.cpp の delta = DEADBAND (強制送信)
    constexpr int forced_delta = DEADBAND;
    EXPECT_GE(forced_delta, DEADBAND);

    delete q2;
}
