/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "after_streaming_fixture.h"
#include "voe_standard_test.h"

class AudioProcessingTest : public AfterStreamingFixture {
 protected:
  // Note: Be careful with this one, it is used in the
  // Android / iPhone part too.
  void TryEnablingAgcWithMode(webrtc::AgcModes agc_mode_to_set) {
    EXPECT_EQ(0, voe_apm_->SetAgcStatus(true, agc_mode_to_set));

    bool agc_enabled = false;
    webrtc::AgcModes agc_mode = webrtc::kAgcDefault;

    EXPECT_EQ(0, voe_apm_->GetAgcStatus(agc_enabled, agc_mode));
    EXPECT_TRUE(agc_enabled);
    EXPECT_EQ(agc_mode_to_set, agc_mode);
  }

  void TryEnablingRxAgcWithMode(webrtc::AgcModes agc_mode_to_set) {
    EXPECT_EQ(0, voe_apm_->SetRxAgcStatus(channel_, true, agc_mode_to_set));

    bool rx_agc_enabled = false;
    webrtc::AgcModes agc_mode = webrtc::kAgcDefault;

    EXPECT_EQ(0, voe_apm_->GetRxAgcStatus(channel_, rx_agc_enabled, agc_mode));
    EXPECT_TRUE(rx_agc_enabled);
    EXPECT_EQ(agc_mode_to_set, agc_mode);
  }

  // EC modes can map to other EC modes, so we have a separate parameter
  // for what we expect the EC mode to be set to.
  void TryEnablingEcWithMode(webrtc::EcModes ec_mode_to_set,
                             webrtc::EcModes expected_mode) {
    EXPECT_EQ(0, voe_apm_->SetEcStatus(true, ec_mode_to_set));

    bool ec_enabled = true;
    webrtc::EcModes ec_mode = webrtc::kEcDefault;

    EXPECT_EQ(0, voe_apm_->GetEcStatus(ec_enabled, ec_mode));

    EXPECT_EQ(expected_mode, ec_mode);
  }

  // Here, the CNG mode will be expected to be on or off depending on the mode.
  void TryEnablingAecmWithMode(webrtc::AecmModes aecm_mode_to_set,
                               bool cng_enabled_to_set) {
    EXPECT_EQ(0, voe_apm_->SetAecmMode(aecm_mode_to_set, cng_enabled_to_set));

    bool cng_enabled = false;
    webrtc::AecmModes aecm_mode = webrtc::kAecmEarpiece;

    voe_apm_->GetAecmMode(aecm_mode, cng_enabled);

    EXPECT_EQ(cng_enabled_to_set, cng_enabled);
    EXPECT_EQ(aecm_mode_to_set, aecm_mode);
  }

  void TryEnablingNsWithMode(webrtc::NsModes ns_mode_to_set,
                             webrtc::NsModes expected_ns_mode) {
    EXPECT_EQ(0, voe_apm_->SetNsStatus(true, ns_mode_to_set));

    bool ns_status = true;
    webrtc::NsModes ns_mode = webrtc::kNsDefault;
    EXPECT_EQ(0, voe_apm_->GetNsStatus(ns_status, ns_mode));

    EXPECT_TRUE(ns_status);
    EXPECT_EQ(expected_ns_mode, ns_mode);
  }

  void TryEnablingRxNsWithMode(webrtc::NsModes ns_mode_to_set,
                               webrtc::NsModes expected_ns_mode) {
    EXPECT_EQ(0, voe_apm_->SetRxNsStatus(channel_, true, ns_mode_to_set));

    bool ns_status = true;
    webrtc::NsModes ns_mode = webrtc::kNsDefault;
    EXPECT_EQ(0, voe_apm_->GetRxNsStatus(channel_, ns_status, ns_mode));

    EXPECT_TRUE(ns_status);
    EXPECT_EQ(expected_ns_mode, ns_mode);
  }

  void TryDetectingSilence() {
    // Here, speech is running. Shut down speech.
    EXPECT_EQ(0, voe_codec_->SetVADStatus(channel_, true));
    EXPECT_EQ(0, voe_volume_control_->SetInputMute(channel_, true));
    EXPECT_EQ(0, voe_file_->StopPlayingFileAsMicrophone(channel_));

    // We should detect the silence after a short time.
    Sleep(50);
    for (int i = 0; i < 25; i++) {
      EXPECT_EQ(0, voe_apm_->VoiceActivityIndicator(channel_));
      Sleep(10);
    }
  }

  void TryDetectingSpeechAfterSilence() {
    // Re-enable speech.
    RestartFakeMicrophone();
    EXPECT_EQ(0, voe_codec_->SetVADStatus(channel_, false));
    EXPECT_EQ(0, voe_volume_control_->SetInputMute(channel_, false));

    // We should detect the speech after a short time.
    for (int i = 0; i < 50; i++) {
      if (voe_apm_->VoiceActivityIndicator(channel_) == 1) {
        return;
      }
      Sleep(10);
    }

    ADD_FAILURE() << "Failed to detect speech within 500 ms.";
  }
};

#if !defined(MAC_IPHONE) && !defined(WEBRTC_ANDROID)

TEST_F(AudioProcessingTest, AgcIsOnByDefault) {
  bool agc_enabled = false;
  webrtc::AgcModes agc_mode = webrtc::kAgcAdaptiveAnalog;

  EXPECT_EQ(0, voe_apm_->GetAgcStatus(agc_enabled, agc_mode));
  EXPECT_TRUE(agc_enabled);
  EXPECT_EQ(webrtc::kAgcAdaptiveAnalog, agc_mode);
}

TEST_F(AudioProcessingTest, CanEnableAgcWithAllModes) {
  TryEnablingAgcWithMode(webrtc::kAgcAdaptiveDigital);
  TryEnablingAgcWithMode(webrtc::kAgcAdaptiveAnalog);
  TryEnablingAgcWithMode(webrtc::kAgcFixedDigital);
}

TEST_F(AudioProcessingTest, EcIsDisabledAndAecIsDefaultEcMode) {
  bool ec_enabled = true;
  webrtc::EcModes ec_mode = webrtc::kEcDefault;

  EXPECT_EQ(0, voe_apm_->GetEcStatus(ec_enabled, ec_mode));
  EXPECT_FALSE(ec_enabled);
  EXPECT_EQ(webrtc::kEcAec, ec_mode);
}

TEST_F(AudioProcessingTest, EnablingEcAecShouldEnableEcAec) {
  TryEnablingEcWithMode(webrtc::kEcAec, webrtc::kEcAec);
}

TEST_F(AudioProcessingTest, EnablingEcConferenceShouldEnableEcAec) {
  TryEnablingEcWithMode(webrtc::kEcConference, webrtc::kEcAec);
}

TEST_F(AudioProcessingTest, EcMetricsAreOffByDefault) {
  bool enabled = true;
  EXPECT_EQ(0, voe_apm_->GetEcMetricsStatus(enabled));
  EXPECT_FALSE(enabled);
}

TEST_F(AudioProcessingTest, ManualTestEcMetrics) {
  SwitchToManualMicrophone();

  EXPECT_EQ(0, voe_apm_->SetEcMetricsStatus(true));

  // Must enable AEC to get valid echo metrics.
  EXPECT_EQ(0, voe_apm_->SetEcStatus(true, webrtc::kEcAec));

  TEST_LOG("Speak into microphone and check metrics for 10 seconds...\n");
  int erl, erle, rerl, a_nlp;
  int delay_median = 0;
  int delay_std = 0;

  for (int i = 0; i < 5; i++) {
    Sleep(2000);
    EXPECT_EQ(0, voe_apm_->GetEchoMetrics(erl, erle, rerl, a_nlp));
    EXPECT_EQ(0, voe_apm_->GetEcDelayMetrics(delay_median, delay_std));
    TEST_LOG("    Echo  : ERL=%5d, ERLE=%5d, RERL=%5d, A_NLP=%5d [dB], "
        " delay median=%3d, delay std=%3d [ms]\n", erl, erle, rerl, a_nlp,
        delay_median, delay_std);
  }

  EXPECT_EQ(0, voe_apm_->SetEcMetricsStatus(false));
}

// TODO(phoglund): Reenable below test when it's no longer flaky.
TEST_F(AudioProcessingTest, DISABLED_TestVoiceActivityDetectionWithObserver) {
  voetest::RxCallback rx_callback;
  EXPECT_EQ(0, voe_apm_->RegisterRxVadObserver(channel_, rx_callback));

  // The extra sleeps are to allow decisions some time to propagate to the
  // observer.
  TryDetectingSilence();
  Sleep(100);

  EXPECT_EQ(0, rx_callback._vadDecision);

  TryDetectingSpeechAfterSilence();
  Sleep(100);

  EXPECT_EQ(1, rx_callback._vadDecision);

  EXPECT_EQ(0, voe_apm_->DeRegisterRxVadObserver(channel_));
}

#endif   // !MAC_IPHONE && !WEBRTC_ANDROID

TEST_F(AudioProcessingTest, EnablingEcAecmShouldEnableEcAecm) {
  // This one apparently applies to Android and iPhone as well.
  TryEnablingEcWithMode(webrtc::kEcAecm, webrtc::kEcAecm);
}

TEST_F(AudioProcessingTest, EcAecmModeIsEnabledAndSpeakerphoneByDefault) {
  bool cng_enabled = false;
  webrtc::AecmModes aecm_mode = webrtc::kAecmEarpiece;

  voe_apm_->GetAecmMode(aecm_mode, cng_enabled);

  EXPECT_TRUE(cng_enabled);
  EXPECT_EQ(webrtc::kAecmSpeakerphone, aecm_mode);
}

TEST_F(AudioProcessingTest, CanSetAecmMode) {
  EXPECT_EQ(0, voe_apm_->SetEcStatus(true, webrtc::kEcAecm));

  // Try some AECM mode - CNG enabled combinations.
  TryEnablingAecmWithMode(webrtc::kAecmEarpiece, true);
  TryEnablingAecmWithMode(webrtc::kAecmEarpiece, false);
  TryEnablingAecmWithMode(webrtc::kAecmLoudEarpiece, true);
  TryEnablingAecmWithMode(webrtc::kAecmLoudSpeakerphone, false);
  TryEnablingAecmWithMode(webrtc::kAecmQuietEarpieceOrHeadset, true);
  TryEnablingAecmWithMode(webrtc::kAecmSpeakerphone, false);
}

TEST_F(AudioProcessingTest, RxAgcShouldBeOffByDefault) {
  bool rx_agc_enabled = true;
  webrtc::AgcModes agc_mode = webrtc::kAgcDefault;

  EXPECT_EQ(0, voe_apm_->GetRxAgcStatus(channel_, rx_agc_enabled, agc_mode));
  EXPECT_FALSE(rx_agc_enabled);
  EXPECT_EQ(webrtc::kAgcAdaptiveDigital, agc_mode);
}

TEST_F(AudioProcessingTest, CanTurnOnDigitalRxAcg) {
  TryEnablingRxAgcWithMode(webrtc::kAgcAdaptiveDigital);
  TryEnablingRxAgcWithMode(webrtc::kAgcFixedDigital);
}

TEST_F(AudioProcessingTest, CannotTurnOnAdaptiveAnalogRxAgc) {
  EXPECT_EQ(-1, voe_apm_->SetRxAgcStatus(
      channel_, true, webrtc::kAgcAdaptiveAnalog));
}

TEST_F(AudioProcessingTest, NsIsOffWithModerateSuppressionByDefault) {
  bool ns_status = true;
  webrtc::NsModes ns_mode = webrtc::kNsDefault;
  EXPECT_EQ(0, voe_apm_->GetNsStatus(ns_status, ns_mode));

  EXPECT_FALSE(ns_status);
  EXPECT_EQ(webrtc::kNsModerateSuppression, ns_mode);
}

TEST_F(AudioProcessingTest, CanSetNsMode) {
  // Concrete suppression values map to themselves.
  TryEnablingNsWithMode(webrtc::kNsHighSuppression,
                        webrtc::kNsHighSuppression);
  TryEnablingNsWithMode(webrtc::kNsLowSuppression,
                        webrtc::kNsLowSuppression);
  TryEnablingNsWithMode(webrtc::kNsModerateSuppression,
                        webrtc::kNsModerateSuppression);
  TryEnablingNsWithMode(webrtc::kNsVeryHighSuppression,
                        webrtc::kNsVeryHighSuppression);

  // Conference and Default map to concrete values.
  TryEnablingNsWithMode(webrtc::kNsConference,
                        webrtc::kNsHighSuppression);
  TryEnablingNsWithMode(webrtc::kNsDefault,
                        webrtc::kNsModerateSuppression);
}

TEST_F(AudioProcessingTest, RxNsIsOffWithModerateSuppressionByDefault) {
  bool ns_status = true;
  webrtc::NsModes ns_mode = webrtc::kNsDefault;
  EXPECT_EQ(0, voe_apm_->GetRxNsStatus(channel_, ns_status, ns_mode));

  EXPECT_FALSE(ns_status);
  EXPECT_EQ(webrtc::kNsModerateSuppression, ns_mode);
}

TEST_F(AudioProcessingTest, CanSetRxNsMode) {
  EXPECT_EQ(0, voe_apm_->SetRxNsStatus(channel_, true));

  // See comments on the regular NS test above.
  TryEnablingRxNsWithMode(webrtc::kNsHighSuppression,
                          webrtc::kNsHighSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsLowSuppression,
                          webrtc::kNsLowSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsModerateSuppression,
                          webrtc::kNsModerateSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsVeryHighSuppression,
                          webrtc::kNsVeryHighSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsConference,
                          webrtc::kNsHighSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsDefault,
                          webrtc::kNsModerateSuppression);
}

TEST_F(AudioProcessingTest, VadIsDisabledByDefault) {
  bool vad_enabled;
  bool disabled_dtx;
  webrtc::VadModes vad_mode;

  EXPECT_EQ(0, voe_codec_->GetVADStatus(
      channel_, vad_enabled, vad_mode, disabled_dtx));

  EXPECT_FALSE(vad_enabled);
}

TEST_F(AudioProcessingTest, VoiceActivityIndicatorReturns1WithSpeechOn) {
  // This sleep is necessary since the voice detection algorithm needs some
  // time to detect the speech from the fake microphone.
  Sleep(500);
  EXPECT_EQ(1, voe_apm_->VoiceActivityIndicator(channel_));
}

#if defined(MAC_IPHONE) || defined(WEBRTC_ANDROID)

TEST_F(AudioProcessingTest, AgcIsOffByDefaultAndDigital) {
  bool agc_enabled = true;
  webrtc::AgcModes agc_mode = webrtc::kAgcAdaptiveAnalog;

  EXPECT_EQ(0, voe_apm_->GetAgcStatus(agc_enabled, agc_mode));
  EXPECT_FALSE(agc_enabled);
  EXPECT_EQ(webrtc::kAgcAdaptiveDigital, agc_mode);
}

TEST_F(AudioProcessingTest, CanEnableAgcInAdaptiveDigitalMode) {
  TryEnablingAgcWithMode(webrtc::kAgcAdaptiveDigital);
}

TEST_F(AudioProcessingTest, AgcIsPossibleExceptInAdaptiveAnalogMode) {
  EXPECT_EQ(-1, voe_apm_->SetAgcStatus(true, webrtc::kAgcAdaptiveAnalog));
  EXPECT_EQ(0, voe_apm_->SetAgcStatus(true, webrtc::kAgcFixedDigital));
  EXPECT_EQ(0, voe_apm_->SetAgcStatus(true, webrtc::kAgcAdaptiveDigital));
}

TEST_F(AudioProcessingTest, EcIsDisabledAndAecmIsDefaultEcMode) {
  bool ec_enabled = true;
  webrtc::EcModes ec_mode = webrtc::kEcDefault;

  EXPECT_EQ(0, voe_apm_->GetEcStatus(ec_enabled, ec_mode));
  EXPECT_FALSE(ec_enabled);
  EXPECT_EQ(webrtc::kEcAecm, ec_mode);
}

TEST_F(AudioProcessingTest, TestVoiceActivityDetection) {
  TryDetectingSilence();
  TryDetectingSpeechAfterSilence();
}

#endif  // MAC_IPHONE || WEBRTC_ANDROID
