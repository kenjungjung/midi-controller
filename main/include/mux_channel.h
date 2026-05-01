#include "mux_controller.h"

/** @brief MuxController の1チャンネルを IAnalogInput として公開するラッパー */
class MuxChannel : public IAnalogInput {
public:
    /** @brief コントローラーとチャンネルインデックスを指定する
     *  @param ctrl   共有 MuxController
     *  @param ch     マルチプレクサチャンネル（0–3）
     */
    MuxChannel(MuxController& ctrl, uint8_t ch);

    /** @brief チャンネルを選択してキャリブレーション済み値（0–4095）を返す */
    uint16_t read() const override;

private:
    MuxController& ctrl_;
    uint8_t        ch_;
};