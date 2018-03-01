/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */



#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <mali_kbase_debug.h>
#include <soc/sprd/sci.h>
#include <soc/sprd/sci_glb_regs.h>
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#ifdef CONFIG_OF
#include <linux/of.h>
#endif

/* Versatile Express (VE) configuration defaults shared between config_attributes[]
 * and config_attributes_hw_issue_8408[]. Settings are not shared for
 * JS_HARD_STOP_TICKS_SS and JS_RESET_TICKS_SS.
 */
#define KBASE_JS_SCHEDULING_TICK_NS_DEBUG    15000000u      /* 15ms, an agressive tick for testing purposes. This will reduce performance significantly */
#define KBASE_JS_SOFT_STOP_TICKS_DEBUG       1	/* between 15ms and 30ms before soft-stop a job */
#define KBASE_JS_SOFT_STOP_TICKS_CL_DEBUG    1	/* between 15ms and 30ms before soft-stop a CL job */
#define KBASE_JS_HARD_STOP_TICKS_SS_DEBUG    333	/* 5s before hard-stop */
#define KBASE_JS_HARD_STOP_TICKS_SS_8401_DEBUG 2000	/* 30s before hard-stop, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) - for issue 8401 */
#define KBASE_JS_HARD_STOP_TICKS_CL_DEBUG    166	/* 2.5s before hard-stop */
#define KBASE_JS_HARD_STOP_TICKS_NSS_DEBUG   100000	/* 1500s (25mins) before NSS hard-stop */
#define KBASE_JS_RESET_TICKS_SS_DEBUG        500	/* 45s before resetting GPU, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) */
#define KBASE_JS_RESET_TICKS_SS_8401_DEBUG   3000	/* 7.5s before resetting GPU - for issue 8401 */
#define KBASE_JS_RESET_TICKS_CL_DEBUG        500	/* 45s before resetting GPU */
#define KBASE_JS_RESET_TICKS_NSS_DEBUG       100166	/* 1502s before resetting GPU */

#define KBASE_JS_SCHEDULING_TICK_NS          1250000000u	/* 1.25s */
#define KBASE_JS_SOFT_STOP_TICKS             2	/* 2.5s before soft-stop a job */
#define KBASE_JS_SOFT_STOP_TICKS_CL          1	/* 1.25s before soft-stop a CL job */
#define KBASE_JS_HARD_STOP_TICKS_SS          4	/* 5s before hard-stop */
#define KBASE_JS_HARD_STOP_TICKS_SS_8401     24	/* 30s before hard-stop, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) - for issue 8401 */
#define KBASE_JS_HARD_STOP_TICKS_CL          2	/* 2.5s before hard-stop */
#define KBASE_JS_HARD_STOP_TICKS_NSS         1200	/* 1500s before NSS hard-stop */
#define KBASE_JS_RESET_TICKS_SS              6	/* 7.5s before resetting GPU */
#define KBASE_JS_RESET_TICKS_SS_8401         36	/* 45s before resetting GPU, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) - for issue 8401 */
#define KBASE_JS_RESET_TICKS_CL              3	/* 7.5s before resetting GPU */
#define KBASE_JS_RESET_TICKS_NSS             1201	/* 1502s before resetting GPU */

#define KBASE_JS_RESET_TIMEOUT_MS            3000	/* 3s before cancelling stuck jobs */
#define KBASE_JS_CTX_TIMESLICE_NS            1000000	/* 1ms - an agressive timeslice for testing purposes (causes lots of scheduling out for >4 ctxs) */
#define KBASE_POWER_MANAGEMENT_CALLBACKS     ((uintptr_t)&pm_sc9838_callbacks)
#define KBASE_PLATFORM_FUNC                  ((uintptr_t)&platform_funcs)

#define HARD_RESET_AT_POWER_OFF 0

#define GPU_GLITCH_FREE_DFS 0
#define UP_THRESHOLD		9/10
#define __SPRD_GPU_TIMEOUT	(3*1000)

struct gpu_freq_info {
	struct clk* clk_src;
	int freq;
	int div;
	int up_threshold;
};

struct gpu_dfs_context {
	int gpu_clock_on;
	int gpu_power_on;
	int cur_load;

	struct clk*  gpu_clock;
	struct clk*  gpu_clock_i;
	struct clk** gpu_clk_src;
	int gpu_clk_num;

	struct gpu_freq_info* freq_list;
	int freq_list_len;

	const struct gpu_freq_info* freq_cur;
	const struct gpu_freq_info* freq_next;

	const struct gpu_freq_info* freq_min;
	const struct gpu_freq_info* freq_max;
	const struct gpu_freq_info* freq_default;
	const struct gpu_freq_info* freq_9;
	const struct gpu_freq_info* freq_8;
	const struct gpu_freq_info* freq_7;
	const struct gpu_freq_info* freq_5;
	const struct gpu_freq_info* freq_range_max;
	const struct gpu_freq_info* freq_range_min;

	struct workqueue_struct *gpu_dfs_workqueue;
	struct semaphore* sem;
};

#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 68,
	.mmu_irq_number = 69,
	.gpu_irq_number = 70,
	.io_memory_region = {
	.start = 0x2F010000,
	.end = 0x2F010000 + (4096 * 4) - 1}
};
#endif

DEFINE_SEMAPHORE(gpu_dfs_sem);
static struct gpu_dfs_context gpu_dfs_ctx=
{
	.gpu_clock_on=0,
	.gpu_power_on=0,

	.sem=&gpu_dfs_sem,
};

extern int gpu_boost_level;
extern int gpu_boost_sf_level;

extern int gpu_freq_cur;
extern int gpu_freq_min_limit;
extern int gpu_freq_max_limit;
extern char* gpu_freq_list;

static void gpu_freq_list_show(char* buf)
{
	int i=0,len=0;

	for(i=0; i<gpu_dfs_ctx.freq_list_len; i++)
	{
		len = sprintf(buf,"%2d  %6d\n", i, gpu_dfs_ctx.freq_list[i].freq);
		buf += len;
	}
}

static inline void mali_freq_init(void)
{
#ifdef CONFIG_OF
	int i = 0, clk_cnt = 0;
	struct device_node *np;
	extern const struct of_device_id kbase_dt_ids[];

	np = of_find_matching_node(NULL, kbase_dt_ids);
	if (!np) {
		printk(KERN_ERR "Mali freqency init fail!\n");
		return;
	}

	gpu_dfs_ctx.gpu_clock_i = of_clk_get(np, 0);
	gpu_dfs_ctx.gpu_clock = of_clk_get(np, 1);
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.gpu_clock_i);
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.gpu_clock);

	clk_cnt = of_property_count_strings(np, "clock-names");
	gpu_dfs_ctx.gpu_clk_num = clk_cnt - 2;
	gpu_dfs_ctx.gpu_clk_src = vmalloc(sizeof(struct clk*) * gpu_dfs_ctx.gpu_clk_num);
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.gpu_clk_src);

	for (i = 0; i < gpu_dfs_ctx.gpu_clk_num; i++) {
		const char *clk_name;

		of_property_read_string_index(np, "clock-names", i+2, &clk_name);
		gpu_dfs_ctx.gpu_clk_src[i] = of_clk_get_by_name(np, clk_name);
		KBASE_DEBUG_ASSERT(gpu_dfs_ctx.gpu_clk_src[i]);
	}

	of_property_read_u32(np, "freq-list-len", &gpu_dfs_ctx.freq_list_len);
	gpu_dfs_ctx.freq_list = vmalloc(sizeof(struct gpu_freq_info) * gpu_dfs_ctx.freq_list_len);
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_list);

	for(i=0; i<gpu_dfs_ctx.freq_list_len; i++)
	{
		int clk = 0;

		of_property_read_u32_index(np, "freq-lists", 3*i+1, &clk);
		gpu_dfs_ctx.freq_list[i].clk_src = gpu_dfs_ctx.gpu_clk_src[clk-2];
		KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_list[i].clk_src);
		of_property_read_u32_index(np, "freq-lists", 3*i,   &gpu_dfs_ctx.freq_list[i].freq);
		of_property_read_u32_index(np, "freq-lists", 3*i+2, &gpu_dfs_ctx.freq_list[i].div);
		gpu_dfs_ctx.freq_list[i].up_threshold =  gpu_dfs_ctx.freq_list[i].freq * UP_THRESHOLD;
	}

	of_property_read_u32(np, "freq-default", &i);
	gpu_dfs_ctx.freq_default = &gpu_dfs_ctx.freq_list[i];
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_default);

	of_property_read_u32(np, "freq-9", &i);
	gpu_dfs_ctx.freq_9 = &gpu_dfs_ctx.freq_list[i];
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_9);

	of_property_read_u32(np, "freq-8", &i);
	gpu_dfs_ctx.freq_8 = &gpu_dfs_ctx.freq_list[i];
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_8);

	of_property_read_u32(np, "freq-7", &i);
	gpu_dfs_ctx.freq_7 = &gpu_dfs_ctx.freq_list[i];
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_7);

	of_property_read_u32(np, "freq-5", &i);
	gpu_dfs_ctx.freq_5 = &gpu_dfs_ctx.freq_list[i];
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_5);

	of_property_read_u32(np, "freq-range-max", &i);
	gpu_dfs_ctx.freq_range_max = &gpu_dfs_ctx.freq_list[i];
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_range_max);

	of_property_read_u32(np, "freq-range-min", &i);
	gpu_dfs_ctx.freq_range_min = &gpu_dfs_ctx.freq_list[i];
	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_range_min);

	gpu_dfs_ctx.freq_max = gpu_dfs_ctx.freq_range_max;
	gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_range_min;
	gpu_dfs_ctx.freq_cur = gpu_dfs_ctx.freq_default;
#endif
}

static inline void mali_power_on(void)
{
	sci_glb_clr(REG_PMU_APB_PD_GPU_TOP_CFG, BIT_PD_GPU_TOP_FORCE_SHUTDOWN);
	udelay(100);
	gpu_dfs_ctx.gpu_power_on = 1;
}

static inline void mali_power_off(void)
{
	gpu_dfs_ctx.gpu_power_on = 0;
	sci_glb_set(REG_PMU_APB_PD_GPU_TOP_CFG, BIT_PD_GPU_TOP_FORCE_SHUTDOWN);
}

static int sprd_gpu_domain_state(void)
{
	/* FIXME: rtc domain */
	u32 power_state1, power_state2, power_state3;
	unsigned long timeout = jiffies + msecs_to_jiffies(__SPRD_GPU_TIMEOUT);

	do {
		cpu_relax();
		power_state1 = sci_glb_read(REG_PMU_APB_PWR_STATUS0_DBG,BITS_PD_GPU_TOP_STATE(-1));
		power_state2 = sci_glb_read(REG_PMU_APB_PWR_STATUS0_DBG,BITS_PD_GPU_TOP_STATE(-1));
		power_state3 = sci_glb_read(REG_PMU_APB_PWR_STATUS0_DBG,BITS_PD_GPU_TOP_STATE(-1));
		if (time_after(jiffies, timeout)) {
			pr_emerg("gpu domain not ready, state %08x %08x\n",
			sci_glb_read(REG_PMU_APB_PWR_STATUS0_DBG,-1),sci_glb_read(REG_AON_APB_APB_EB0,-1));
		}
	} while (power_state1 != power_state2 || power_state2 != power_state3);

	return (power_state1);
}

static void sprd_gpu_domain_wait_for_ready(void)
{
	int timeout_count = 2000;

	while(sprd_gpu_domain_state() != BITS_PD_GPU_TOP_STATE(0))
	{
		if(0 == timeout_count)
		{
			pr_emerg("gpu domain is not ready for too long time, state %08x %08x\n",
			sci_glb_read(REG_PMU_APB_PWR_STATUS0_DBG,-1),sci_glb_read(REG_AON_APB_APB_EB0,-1));
			return;
		}
		udelay(50);
		timeout_count--;
	}
}

static inline void mali_set_div(int clock_div)
{
	sci_glb_write(REG_GPU_APB_APB_CLK_CTRL,BITS_CLK_GPU_DIV(clock_div-1),BITS_CLK_GPU_DIV(3));
}

static inline void mali_clock_on(void)
{
	int i;

	//enable all clocks
	for(i=0;i<gpu_dfs_ctx.gpu_clk_num;i++)
	{
		clk_prepare_enable(gpu_dfs_ctx.gpu_clk_src[i]);
	}

	//enable gpu clock i
	clk_prepare_enable(gpu_dfs_ctx.gpu_clock_i);

	sprd_gpu_domain_wait_for_ready();

	//set gpu clock parent
	clk_set_parent(gpu_dfs_ctx.gpu_clock, gpu_dfs_ctx.freq_default->clk_src);

	KBASE_DEBUG_ASSERT(gpu_dfs_ctx.freq_cur);
	clk_set_parent(gpu_dfs_ctx.gpu_clock, gpu_dfs_ctx.freq_cur->clk_src);
	mali_set_div(gpu_dfs_ctx.freq_cur->div);

	//enable gpu clock
	clk_prepare_enable(gpu_dfs_ctx.gpu_clock);
	udelay(100);

	gpu_dfs_ctx.gpu_clock_on = 1;

	gpu_freq_cur = gpu_dfs_ctx.freq_cur->freq;
}

static inline void mali_clock_off(void)
{
	int i;

	gpu_freq_cur = 0;

	gpu_dfs_ctx.gpu_clock_on = 0;

	//disable gpu clock and i
	clk_disable_unprepare(gpu_dfs_ctx.gpu_clock);
	clk_disable_unprepare(gpu_dfs_ctx.gpu_clock_i);

	//disable all clocks
	for(i=0;i<gpu_dfs_ctx.gpu_clk_num;i++)
	{
		clk_disable_unprepare(gpu_dfs_ctx.gpu_clk_src[i]);
	}
}

static mali_bool mali_platform_init(struct kbase_device *kbdev)
{
	printk(KERN_ERR "mali------------------- mali_platform_init\n");
	//gpu freq
	mali_freq_init();

	//power on
	sci_glb_write(REG_PMU_APB_PD_GPU_TOP_CFG,BITS_PD_GPU_TOP_PWR_ON_DLY(1),0xFF0000);
	sci_glb_write(REG_PMU_APB_PD_GPU_TOP_CFG,BITS_PD_GPU_TOP_PWR_ON_SEQ_DLY(1),0xFF00);
	sci_glb_write(REG_PMU_APB_PD_GPU_TOP_CFG,BITS_PD_GPU_TOP_ISO_ON_DLY(1),0xFF);

	mali_power_on();

	//clock on
	mali_clock_on();

	gpu_dfs_ctx.gpu_dfs_workqueue = create_singlethread_workqueue("gpu_dfs");

	gpu_freq_list = (char*)vmalloc(256*sizeof(char));
	gpu_freq_list_show(gpu_freq_list);

	return MALI_TRUE;
}

static void mali_platform_term(struct kbase_device *kbdev)
{
	printk(KERN_ERR "mali------------------- mali_platform_term\n");
	//destory work queue
	destroy_workqueue(gpu_dfs_ctx.gpu_dfs_workqueue);

	//clock off
	mali_clock_off();

	//power off
	mali_power_off();

	//free
	vfree(gpu_freq_list);
	vfree(gpu_dfs_ctx.freq_list);
	vfree(gpu_dfs_ctx.gpu_clk_src);
}

static struct kbase_platform_funcs_conf platform_funcs = {
	.platform_init_func = mali_platform_init,
	.platform_term_func = mali_platform_term
};

static void mali_power_mode_change(int power_mode)
{
	down(gpu_dfs_ctx.sem);
	//printk(KERN_ERR "Mali power mode change %d, gpu_power_on=%d gpu_clock_on=%d\n",
	//	    power_mode,gpu_dfs_ctx.gpu_power_on,gpu_dfs_ctx.gpu_clock_on);
	switch (power_mode)
	{
	case 0://power on
		if (!gpu_dfs_ctx.gpu_power_on)
		{
			gpu_dfs_ctx.freq_cur = gpu_dfs_ctx.freq_max;

			mali_power_on();
			mali_clock_on();
		}

		if (!gpu_dfs_ctx.gpu_clock_on)
		{
			mali_clock_on();
		}
		break;

	case 1://light sleep
	case 2://deep sleep
		if(gpu_dfs_ctx.gpu_clock_on)
		{
			mali_clock_off();
		}

		if(gpu_dfs_ctx.gpu_power_on)
		{
			mali_power_off();
		}
		break;

	default:
		break;
	}
	up(gpu_dfs_ctx.sem);
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
//	printk(KERN_ERR "mali------------------- pm_callback_power_off\n");
	mali_power_mode_change(1);
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
//	printk(KERN_ERR "mali------------------- pm_callback_power_on\n");
	mali_power_mode_change(0);
	return 1;
}

static void pm_callback_power_suspend(struct kbase_device *kbdev)
{
//	printk(KERN_ERR "mali------------------- pm_callback_power_suspend\n");
	mali_power_mode_change(2);
}

static void pm_callback_power_resume(struct kbase_device *kbdev)
{
//	printk(KERN_ERR "mali------------------- pm_callback_power_resume\n");
	mali_power_mode_change(0);
}

static mali_error pm_callback_power_runtime_init(struct kbase_device *kbdev)
{
	printk(KERN_ERR "mali------------------- pm_callback_power_runtime_init\n");
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_set_active(kbdev->dev);
	pm_suspend_ignore_children(kbdev->dev, true);
	pm_runtime_set_autosuspend_delay(kbdev->dev, 50);
	pm_runtime_use_autosuspend(kbdev->dev);
	pm_runtime_enable(kbdev->dev);
#endif
	return MALI_ERROR_NONE;
}

static void pm_callback_power_runtime_term(struct kbase_device *kbdev)
{
//	printk(KERN_ERR "mali------------------- pm_callback_power_runtime_term\n");
}

static void pm_callback_power_runtime_off(struct kbase_device *kbdev)
{
//	printk(KERN_ERR "mali------------------- pm_callback_power_runtime_off\n");
}

static int pm_callback_power_runtime_on(struct kbase_device *kbdev)
{
//	printk(KERN_ERR "mali------------------- pm_callback_power_runtime_on\n");
	return 1;
}

static struct kbase_pm_callback_conf pm_sc9838_callbacks = {
	.power_off_callback = pm_callback_power_off,
	.power_on_callback = pm_callback_power_on,
	.power_suspend_callback = pm_callback_power_suspend,
	.power_resume_callback = pm_callback_power_resume,
	.power_runtime_init_callback = pm_callback_power_runtime_init,
	.power_runtime_term_callback = pm_callback_power_runtime_term,
	.power_runtime_off_callback = pm_callback_power_runtime_off,
	.power_runtime_on_callback = pm_callback_power_runtime_on
};

/* Please keep table config_attributes in sync with config_attributes_hw_issue_8408 */
static struct kbase_attribute config_attributes[] = {
#ifdef CONFIG_MALI_DEBUG
/* Use more aggressive scheduling timeouts in debug builds for testing purposes */
	{
	 KBASE_CONFIG_ATTR_JS_SCHEDULING_TICK_NS,
	 KBASE_JS_SCHEDULING_TICK_NS_DEBUG},

	{
	 KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS,
	 KBASE_JS_SOFT_STOP_TICKS_DEBUG},

	{
	 KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS_CL,
	 KBASE_JS_SOFT_STOP_TICKS_CL_DEBUG},

	{
	 KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_SS,
	 KBASE_JS_HARD_STOP_TICKS_SS_DEBUG},

	{
	 KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_CL,
	 KBASE_JS_HARD_STOP_TICKS_CL_DEBUG},

	{
	 KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_NSS,
	 KBASE_JS_HARD_STOP_TICKS_NSS_DEBUG},

	{
	 KBASE_CONFIG_ATTR_JS_RESET_TICKS_SS,
	 KBASE_JS_RESET_TICKS_SS_DEBUG},

	{
	 KBASE_CONFIG_ATTR_JS_RESET_TICKS_CL,
	 KBASE_JS_RESET_TICKS_CL_DEBUG},

	{
	 KBASE_CONFIG_ATTR_JS_RESET_TICKS_NSS,
	 KBASE_JS_RESET_TICKS_NSS_DEBUG},
#else				/* CONFIG_MALI_DEBUG */
/* In release builds same as the defaults but scaled for 5MHz FPGA */
	{
	 KBASE_CONFIG_ATTR_JS_SCHEDULING_TICK_NS,
	 KBASE_JS_SCHEDULING_TICK_NS},

	{
	 KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS,
	 KBASE_JS_SOFT_STOP_TICKS},

	{
	 KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS_CL,
	 KBASE_JS_SOFT_STOP_TICKS_CL},

	{
	 KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_SS,
	 KBASE_JS_HARD_STOP_TICKS_SS},

	{
	 KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_CL,
	 KBASE_JS_HARD_STOP_TICKS_CL},

	{
	 KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_NSS,
	 KBASE_JS_HARD_STOP_TICKS_NSS},

	{
	 KBASE_CONFIG_ATTR_JS_RESET_TICKS_SS,
	 KBASE_JS_RESET_TICKS_SS},

	{
	 KBASE_CONFIG_ATTR_JS_RESET_TICKS_CL,
	 KBASE_JS_RESET_TICKS_CL},

	{
	 KBASE_CONFIG_ATTR_JS_RESET_TICKS_NSS,
	 KBASE_JS_RESET_TICKS_NSS},
#endif				/* CONFIG_MALI_DEBUG */
	{
	 KBASE_CONFIG_ATTR_JS_RESET_TIMEOUT_MS,
	 KBASE_JS_RESET_TIMEOUT_MS},

	{
	 KBASE_CONFIG_ATTR_JS_CTX_TIMESLICE_NS,
	 KBASE_JS_CTX_TIMESLICE_NS},

	{
	 KBASE_CONFIG_ATTR_PLATFORM_FUNCS,
	 KBASE_PLATFORM_FUNC},

	{
	 KBASE_CONFIG_ATTR_POWER_MANAGEMENT_CALLBACKS,
	 KBASE_POWER_MANAGEMENT_CALLBACKS},

	{
	 KBASE_CONFIG_ATTR_END,
	 0}
};


static struct kbase_platform_config versatile_platform_config = {
	.attributes = config_attributes,
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}

#ifdef CONFIG_MALI_MIDGARD_DVFS
static int freq_search(struct gpu_freq_info freq_list[], int len, int key)
{
	int low=0, high=len-1, mid;

	if (0 > key)
	{
		return -1;
	}

	while(low <= high)
	{
		mid = (low+high)/2;
		if(key == freq_list[mid].freq)
		{
			return mid;
		}

		if(key < freq_list[mid].freq)
		{
			high = mid-1;
		}
		else
		{
			low = mid+1;
		}
	}
	return -1;
}

static const struct gpu_freq_info* get_next_freq(const struct gpu_freq_info* min_freq, const struct gpu_freq_info* max_freq, int target)
{
	const struct gpu_freq_info* freq;

	for (freq = min_freq; freq <= max_freq; freq++)
	{
		if (freq->up_threshold > target)
		{
			return freq;
		}
	}
	return max_freq;
}

static void gpu_dfs_func(struct work_struct *work)
{
	down(gpu_dfs_ctx.sem);
	if(gpu_dfs_ctx.gpu_power_on && gpu_dfs_ctx.gpu_clock_on)
	{
#if GPU_GLITCH_FREE_DFS
		if(gpu_dfs_ctx.freq_next != gpu_dfs_ctx.freq_cur)
		{
			if(gpu_dfs_ctx.freq_next->clk_src != gpu_dfs_ctx.freq_cur->clk_src)
			{
				clk_set_parent(gpu_dfs_ctx.gpu_clock, gpu_dfs_ctx.freq_next->clk_src);
			}
			if(gpu_dfs_ctx.freq_next->div != gpu_dfs_ctx.freq_cur->div)
			{
				mali_set_div(gpu_dfs_ctx.freq_next->div);
			}

			gpu_dfs_ctx.freq_cur = gpu_dfs_ctx.freq_next;
			gpu_freq_cur = gpu_dfs_ctx.freq_cur->freq;
		}
#else
		if(gpu_dfs_ctx.freq_next != gpu_dfs_ctx.freq_cur)
		{
			clk_disable_unprepare(gpu_dfs_ctx.gpu_clock);

			if(gpu_dfs_ctx.freq_next->clk_src != gpu_dfs_ctx.freq_cur->clk_src)
			{
				clk_set_parent(gpu_dfs_ctx.gpu_clock, gpu_dfs_ctx.freq_next->clk_src);
			}
			if(gpu_dfs_ctx.freq_next->div != gpu_dfs_ctx.freq_cur->div)
			{
				mali_set_div(gpu_dfs_ctx.freq_next->div);
			}

			gpu_dfs_ctx.freq_cur = gpu_dfs_ctx.freq_next;
			gpu_freq_cur = gpu_dfs_ctx.freq_cur->freq;

			clk_prepare_enable(gpu_dfs_ctx.gpu_clock);
			udelay(100);
		}
#endif
	}
	up(gpu_dfs_ctx.sem);
}


static DECLARE_WORK(gpu_dfs_work, &gpu_dfs_func);

int kbase_platform_dvfs_event(struct kbase_device *kbdev, u32 utilisation,
	u32 util_gl_share, u32 util_cl_share[2])
{
	int result = 0, min_index = -1, max_index = -1;

	gpu_dfs_ctx.cur_load = utilisation;

	//printk(KERN_ERR "MALI_DVFS utilisation=%d util_gl_share=%d, util_cl_share[0]=%d, util_cl_share[1]=%d \n",
	//	utilisation, util_gl_share, util_cl_share[0], util_cl_share[1]);
	//printk(KERN_ERR "MALI_DVFS gpu_boost_level:%d gpu_boost_sf_level:%d \n", gpu_boost_level,gpu_boost_sf_level);

	switch(gpu_boost_level)
	{
	case 10:
		gpu_dfs_ctx.freq_max =
		gpu_dfs_ctx.freq_min = &gpu_dfs_ctx.freq_list[gpu_dfs_ctx.freq_list_len-1];
		break;

	case 9:
		gpu_dfs_ctx.freq_max =
		gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_9;
		break;

	case 7:
		gpu_dfs_ctx.freq_max =
		gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_7;
		break;

	case 5:
		gpu_dfs_ctx.freq_max =
		gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_5;
		break;

	case 0:
	default:
		gpu_dfs_ctx.freq_max = gpu_dfs_ctx.freq_range_max;
		gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_range_min;
		break;
	}

	if ((0 == gpu_boost_level) && (0 < gpu_boost_sf_level))
	{
		gpu_dfs_ctx.freq_max =
		gpu_dfs_ctx.freq_min = &gpu_dfs_ctx.freq_list[gpu_dfs_ctx.freq_list_len-1];
	}

	gpu_boost_level = 0;
	gpu_boost_sf_level = 0;

	//limit min freq
	min_index = freq_search(gpu_dfs_ctx.freq_list, gpu_dfs_ctx.freq_list_len, gpu_freq_min_limit);
	if ((0 <= min_index) &&
		(gpu_dfs_ctx.freq_min->freq < gpu_dfs_ctx.freq_list[min_index].freq))
	{
		gpu_dfs_ctx.freq_min = &gpu_dfs_ctx.freq_list[min_index];
		if (gpu_dfs_ctx.freq_min->freq > gpu_dfs_ctx.freq_max->freq)
		{
			gpu_dfs_ctx.freq_max = gpu_dfs_ctx.freq_min;
		}
	}

    //limit max freq
	max_index = freq_search(gpu_dfs_ctx.freq_list, gpu_dfs_ctx.freq_list_len, gpu_freq_max_limit);
	if ((0 <= max_index) &&
		(gpu_dfs_ctx.freq_max->freq > gpu_dfs_ctx.freq_list[max_index].freq))
	{
		gpu_dfs_ctx.freq_max = &gpu_dfs_ctx.freq_list[max_index];
		if (gpu_dfs_ctx.freq_max->freq < gpu_dfs_ctx.freq_min->freq)
		{
			gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_max;
		}
	}

	// if the loading ratio is greater then 90%, switch the clock to the maximum
	if(gpu_dfs_ctx.cur_load >= (100*UP_THRESHOLD))
	{
		gpu_dfs_ctx.freq_next = gpu_dfs_ctx.freq_max;
	}
	else
	{
		int target_freq = gpu_dfs_ctx.freq_cur->freq * gpu_dfs_ctx.cur_load / 256;
		gpu_dfs_ctx.freq_next = get_next_freq(gpu_dfs_ctx.freq_min, gpu_dfs_ctx.freq_max, target_freq);
	}

	printk(KERN_ERR "MALI_DVFS util %3d; cur_freq %6d -> next_freq %6d\n",
		gpu_dfs_ctx.cur_load, gpu_dfs_ctx.freq_cur->freq, gpu_dfs_ctx.freq_next->freq);

	if(gpu_dfs_ctx.freq_next->freq != gpu_dfs_ctx.freq_cur->freq)
	{
		queue_work(gpu_dfs_ctx.gpu_dfs_workqueue, &gpu_dfs_work);
	}

	return (result);
}
#endif
