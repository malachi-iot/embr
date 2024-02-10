# esp-helper: Kconfig

Document v0.1

# Detail

Component registers itself via `Kconfig`, so regular `esp.py menuconfig` picks it up.
Component is seen as:

# 1. RESERVED

# 2. Usage

![top level config](img/menuconfig1.png)

On selecting, some options are presented:

![helper config](img/menuconfig-helper.png)

#### 2.2.1. WiFi Credentials

Pretty self explanatory.  Picked up automatically by `esp_helper`'s
wifi_init_sta() method.

Any more exotic provisioning is outside the scope
of this mechanism, and may conflict.

#### 2.2.2. Target Board

Many development boards are present.  If yours is available,
select it.  Note that `idf.py set-target` is required to select
the desired ESP32 architecture, and boards are filtered by that.

![target board](img/menuconfig_s3_target.png)

#### 2.2.3. SD Card

Specifies SDMMC pin mappings.  Auto populates if board type is selected*

> This one is fiddly.  It only works right when target board from 2.2.1.
> is *already selected* when menuconfig starts.  When in doubt,
> select desired target board and restart menuconfig.

