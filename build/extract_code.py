#!/usr/bin/python3

# Function: extract hi3861 related code from OpenHarmony full-code repository LTS v3.0.5
# OpenHarmony LTS v3.0.5 code source URL: https://gitee.com/link?target=https%3A%2F%2Frepo.huaweicloud.com%2Fopenharmony%2Fos%2F3.0.5%2Fcode-v3.0.5-LTS.tar.gz
# 
# 2022-09-14
# supplement feature components: histreaming, lwip, coap, mqtt from v3.2 Beta3:
# git clone -b OpenHarmony-3.2-Beta3 https://gitee.com/openharmony/device_soc_hisilicon.git OpenHarmony-3.2-Beta3 && cd OpenHarmony-3.2-Beta3 && git lfs pull

import sys, os, subprocess

location_from = "/mnt/local/ohos_3861/project_iot_3861/code_from/"
location_src = os.path.join(location_from, "code-v3.0.5-LTS/OpenHarmony")
location_demo = os.path.join(location_from, "demo")
location_notice = os.path.join(location_from, "NOTICE")
location_tar = "/mnt/local/ohos_3861/project_iot_3861/code_tar"

dirs_to_del_recur = (".git", ".gitee", ".gitattributes", ".gitignore")
dirs_to_del_sep = ("device/hisilicon/hispark_pegasus/sdk_liteos/third_party/u-boot-v2019.07/u-boot-v2019.07",
            "device/hisilicon/hispark_pegasus/sdk_liteos/third_party/u-boot-v2019.07/u-boot-v2019.07/u-boot-v2019.07.tar.gz",
            "device/hisilicon/hispark_pegasus/sdk_liteos/output",
        )
nessesary_branches = (
        "base/hiviewdfx/",
        "base/security/",
        "base/startup/",
        "base/update/ota_lite/",
        "base/global/i18n_standard/",
        "base/global/i18n_lite/",
        "base/global/resmgr_lite/",
        "base/iot_hardware/",
        "base/powermgr/battery_lite/",
        "base/powermgr/powermgr_lite/",
        "base/sensors/",
        "device/hisilicon/hispark_pegasus/",
        "foundation/appexecfwk/appexecfwk_lite",
        "foundation/communication/softbus_lite",
        "foundation/communication/wifi",
        "foundation/communication/wifi_aware",
        "foundation/communication/wifi_lite",
        "foundation/distributeddatamgr/appdatamgr",
        "foundation/distributedschedule/samgr_lite",
        "test/xts/acts/build_lite",
        "test/xts/acts/communication_lite",
        "test/xts/acts/utils_lite",
        "test/xts/acts/startup_lite",
        "test/xts/acts/iot_hardware_lite",
        "test/xts/acts/security_lite",
        "test/xts/acts/hiviewdfx_lite",
        "test/xts/acts/distributed_schedule_lite",
        "test/xts/acts/update_lite",
        "test/xts/tools/",
        "test/xdevice/",
        "third_party/cJSON/",
        "third_party/jinja2/",
        "third_party/mbedtls/",
        "third_party/unity/",
        "utils/native/lite/",
        "vendor/hisilicon/hispark_pegasus/",
        "build/",
        "applications/sample/wifi-iot/",
        "domains/iot/",
        "prebuilts/build-tools/linux-x86/",
        "prebuilts/lite/sysroot/",
        "productdefine/common/",
        )

location_src_supplement = os.path.join(location_from, "code-v3.2-Beta3/hi3861v100/sdk_liteos")
location_tar_supplement = os.path.join(location_tar, "device/hisilicon/hispark_pegasus/sdk_liteos")
branches_supplement = (
        "components/histreaming",
        "third_party/lwip_sack",
        "third_party/libcoap",
        "third_party/paho.mqtt.c",
        "build/libs/libhistreaminglink.a",
        "build/libs/liblwip.a",
        "build/libs/libcoap.a",
        "build/libs/libmqtt.a",
        "SConstruct",
        )

def main():
    def exec_cmd(cmd):
        print(cmd)
        try:
            p = subprocess.run(cmd, shell=True)
            if p.returncode != 0:
                sys.exit(p.returncode)
        except Exception as e:
            print(e)
            sys.exit(p.returncode)

    # pick hi3861 necessary branches to target
    print("--- 1. pick necessary branches from OpenHarmony full code repository to target folder:")
    cmd_rsync = ['rsync -aR {:s} {:s}'.format(branch, location_tar) \
                    for branch in nessesary_branches]
    cmd = 'cd {:s} && '.format(location_src) + ' && '.join(cmd_rsync)
    exec_cmd(cmd)

    # merge demo to target
    print("--- 2. copy demo to target folder:")
    cmd = "rsync -a {:s} {:s}".format(location_demo, os.path.join(location_tar, 'vendor/hisilicon/hispark_pegasus'))
    exec_cmd(cmd)

    # copy Notice and License File to target
    print("--- 3. copy notice to target folder:")
    cmd = "rsync -a {:s} {:s}".format(location_notice, location_tar)
    exec_cmd(cmd)

    # copy supplement to target
    print("--- 4. copy supplements to target folder:")
    cmd_rsync = ['rsync -aR {:s} {:s}'.format(branch, location_tar_supplement) \
                    for branch in branches_supplement]
    cmd = 'cd {:s} && '.format(location_src_supplement) + ' && '.join(cmd_rsync)
    exec_cmd(cmd)
    
    # rm .git .gitee files from target folder
    print("--- 5. delete redundant files")
    cmd_del = ['find -name {} | xargs rm -rf'.format(dd) for dd in dirs_to_del_recur]
    cmd = "cd {} && ".format(location_tar) + " && ".join(cmd_del)
    exec_cmd(cmd)

    cmd_del = ['rm -rf {}'.format(dd) for dd in dirs_to_del_sep]
    cmd = "cd {} && ".format(location_tar) + " && ".join(cmd_del)
    exec_cmd(cmd)

    # keep device\hisilicon\hispark_pegasus\sdk_liteos\3rd_sdk\demolink\libs, otherwise build would fail
    print("--- 6. keep empty folder")
    cmd = "cd {} && touch device/hisilicon/hispark_pegasus/sdk_liteos/3rd_sdk/demolink/libs/.gitkeep".format(location_tar)
    exec_cmd(cmd)

    # replace vendor/hisilicon/hispark_pegasus/config.json to cancel XTS test
    print("--- 7. replace config.json")
    cmd = "cd {} && cp -f config.json {}/vendor/hisilicon/hispark_pegasus".format(location_from, location_tar)
    exec_cmd(cmd)
    
    return 0
    


if "__main__" == __name__:
    sys.exit(main())
