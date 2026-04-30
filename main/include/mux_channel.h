#include "mux_controller.h"

/** @brief MuxController の1チャンネルを IAnalogInput として公開するラッパー */
class MuxChannel : public IAnalogInput {
public:
    /** @brief コントローラーとチャンネルインデックスを指定する
     *  @param ctrl   共有 MuxController
     *  @param ch     マルチプレクサチャンネル（0–3）
     */
    MuxChannel(MuxController& ctrl, uint8_t ch, int vol_min, int vol_max);

    /** @brief チャンネルを選択してキャリブレーション済み値（0–4095）を返す */
    uint16_t read_midi_cc() override;

private:
    MuxController& ctrl_;
    uint8_t        ch_;
    int            vol_min_;
    int            vol_max_;
    
    int            vol_pre_; ////< ロガー用
};