// SPDX-License-Identifier: GPL-2.0
//
// Minimal ASoC card for Raspberry Pi I2S using dummy codec(s)
// Compatible with Linux 6.12 ASoC APIs (snd_soc_dai_link .cpus/.codecs arrays)
//
// Usage (default CPU DAI is for Pi2/3): insmod my_loader.ko cpu_dai="3f203000.i2s"
// For Pi4: insmod my_loader.ko cpu_dai="fe203000.i2s"
//
// This registers a tiny ASoC machine driver with two links:
//   - Playback: I2S -> snd-soc-dummy
//   - Capture : I2S <- snd-soc-dummy
// Later можно заменить dummy на реальный кодек или перейти на DT.

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <sound/soc.h>

MODULE_DESCRIPTION("Raspberry Pi I2S simple ASoC card (dummy codecs) for 6.12");
MODULE_AUTHOR("Based on simplecodec3 idea; adapted by ChatGPT");
MODULE_LICENSE("GPL");

static char *cpu_dai = "3f203000.i2s"; /* Pi2/3 */
module_param(cpu_dai, charp, 0444);
MODULE_PARM_DESC(cpu_dai, "CPU DAI name (e.g. 3f203000.i2s for Pi3, fe203000.i2s for Pi4)");

static struct snd_soc_dai_link_component cpu_comp[1];
static struct snd_soc_dai_link_component dummy_codec_comp[1] = {
    {
        .name = "snd-soc-dummy",
        .dai_name = "snd-soc-dummy-dai",
    },
};

/* Playback and Capture DAI links (modern ASoC layout) */
static struct snd_soc_dai_link dai_links[] = {
    {
        .name = "rpi-i2s-playback",
        .stream_name = "Playback",
        .cpus = cpu_comp,
        .num_cpus = 1,
        .codecs = dummy_codec_comp,
        .num_codecs = 1,
        .dai_fmt = SND_SOC_DAIFMT_I2S |
                   SND_SOC_DAIFMT_NB_NF |
                   SND_SOC_DAIFMT_CBS_CFS,
        .playback_only = 1,
    },
    {
        .name = "rpi-i2s-capture",
        .stream_name = "Capture",
        .cpus = cpu_comp,
        .num_cpus = 1,
        .codecs = dummy_codec_comp,
        .num_codecs = 1,
        .dai_fmt = SND_SOC_DAIFMT_I2S |
                   SND_SOC_DAIFMT_NB_NF |
                   SND_SOC_DAIFMT_CBS_CFS,
        .capture_only = 1,
    },
};

static struct snd_soc_card rpi_card = {
    .name = "rpi-i2s-dummycard",
    .owner = THIS_MODULE,
    .dai_link = dai_links,
    .num_links = ARRAY_SIZE(dai_links),
};

static int rpi_i2s_probe(struct platform_device *pdev)
{
    cpu_comp[0].dai_name = cpu_dai;
    rpi_card.dev = &pdev->dev;
    return devm_snd_soc_register_card(&pdev->dev, &rpi_card);
}

static void rpi_i2s_remove(struct platform_device *pdev)
{
}

static struct platform_driver rpi_i2s_drv = {
    .probe  = rpi_i2s_probe,
    .remove = rpi_i2s_remove,
    .driver = {
        .name = "rpi-i2s-dummydrv",
    },
};

static struct platform_device *rpi_i2s_pdev;

static int __init rpi_i2s_init(void)
{
    int ret;

    ret = platform_driver_register(&rpi_i2s_drv);
    if (ret)
        return ret;

    rpi_i2s_pdev = platform_device_register_simple("rpi-i2s-dummydrv", -1, NULL, 0);
    if (IS_ERR(rpi_i2s_pdev)) {
        platform_driver_unregister(&rpi_i2s_drv);
        return PTR_ERR(rpi_i2s_pdev);
    }

    pr_info("rpi-i2s: registered with cpu_dai=\"%s\"\n", cpu_dai);
    return 0;
}

static void __exit rpi_i2s_exit(void)
{
    if (!IS_ERR_OR_NULL(rpi_i2s_pdev))
        platform_device_unregister(rpi_i2s_pdev);

    platform_driver_unregister(&rpi_i2s_drv);
}

module_init(rpi_i2s_init);
module_exit(rpi_i2s_exit);
