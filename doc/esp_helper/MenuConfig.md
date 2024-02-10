# esp-helper: Kconfig

Describes how to use the `Kconfig` feature of esp-helper

Document v0.1

# Detail

Component registers itself via `Kconfig`, so regular `esp.py menuconfig` picks it up.
Component is seen as:

![top level config](img/menuconfig1.png)

For overall `esp_helper` documentation,
[click here](README.md)

# 1. Design Goals

High level system-wide configuration pertaining to a particular board and its
peripherals.

Note that board selection in section 2.2. cascades out to `board::traits`
mechanism.

# 2. Usage

On selecting, some options are presented:

![helper config](img/menuconfig-helper.png)

#### 2.1. WiFi Credentials

Pretty self explanatory.  Picked up automatically by `esp_helper`'s
wifi_init_sta() method.

Any more exotic provisioning is outside the scope
of this mechanism, and may conflict.

#### 2.2. Target Board

Many development boards are present.  If yours is available,
select it.  Note that `idf.py set-target` is required to select
the desired ESP32 architecture, and boards are filtered by that.

![target board](img/menuconfig_s3_target.png)

#### 2.3. TWAI

#### 2.4. SD Card

Specifies SDMMC pin mappings.  Auto populates if board type is selected*

> This one is fiddly.  It only works right when target board from 2.2.1.
> is *already selected* when menuconfig starts.  When in doubt,
> select desired target board and restart menuconfig.

