---
tags: [moc]
---
# Embedded ML MOC

Getting the detector onto the [[S32K3]] (Cortex-M7, **no NPU** — int8 + CMSIS-NN): train in [[PyTorch]] → convert with [[litert-torch]] →
[[int8]] [[quantization]] → run with [[LiteRT for Microcontrollers]] under [[NXP eIQ]] tooling.
The [[S32K1]] nodes never run ML — only cheap DSP feature extraction (RMS, FFT bands) before [[CAN FD]] transmit.

Key papers: [[David 2020 — TensorFlow Lite Micro]], [[Lin 2020 — MCUNet]], [[Jacob 2017 — Integer Quantization]],
[[Banbury 2021 — MLPerf Tiny]], [[Abadade 2023 — TinyML Review]].
