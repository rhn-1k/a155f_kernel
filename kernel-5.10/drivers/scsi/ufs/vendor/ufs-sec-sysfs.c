// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Specific feature : sysfs-nodes
 *
 * Copyright (C) 2021 Samsung Electronics Co., Ltd.
 *
 * Authors:
 *	Storage Driver <storage.sec@samsung.com>
 */

#include <linux/sysfs.h>
#include <linux/sec_class.h>
#include <linux/sec_debug.h>

#include "ufs-sec-sysfs.h"
#include "ufs-mediatek.h"

/* sec specific vendor sysfs nodes */
static struct device *sec_ufs_cmd_dev;

static ssize_t ufs_unique_number_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", ufs_vdi.unique_number);
}
static DEVICE_ATTR(un, 0440, ufs_unique_number_show, NULL);

static ssize_t ufs_lt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ufs_hba *hba = dev_get_drvdata(dev);

	if (!hba) {
		dev_err(dev, "skipping ufs lt read\n");
		ufs_vdi.lt = 0;
	} else if (hba->ufshcd_state == UFSHCD_STATE_OPERATIONAL) {
		pm_runtime_get_sync(hba->dev);
		ufs_sec_get_health_desc(hba);
		pm_runtime_put(hba->dev);
	} else {
		/* return previous LT value if not operational */
		dev_info(hba->dev, "ufshcd_state : %d, old LT: %01x\n",
				hba->ufshcd_state, ufs_vdi.lt);
	}
	return snprintf(buf, PAGE_SIZE, "%01x\n", ufs_vdi.lt);
}
static DEVICE_ATTR(lt, 0444, ufs_lt_show, NULL);

static ssize_t ufs_sec_flt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ufs_hba *hba = ufs_vdi.hba;

	if (!hba) {
		dev_err(dev, "skipping ufs flt read\n");
		ufs_vdi.flt = 0;
	} else if (hba->ufshcd_state == UFSHCD_STATE_OPERATIONAL) {
		pm_runtime_get_sync(hba->dev);
		ufs_sec_get_health_desc(hba);
		pm_runtime_put(hba->dev);
	} else {
		/* return previous FLT value if not operational */
		dev_info(hba->dev, "ufshcd_state : %d, old FLT: %u\n",
				hba->ufshcd_state, ufs_vdi.flt);
	}

	return snprintf(buf, PAGE_SIZE, "%u\n", ufs_vdi.flt);
}
static DEVICE_ATTR(flt, 0444, ufs_sec_flt_show, NULL);

static ssize_t ufs_sec_eli_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ufs_hba *hba = ufs_vdi.hba;

	if (!hba) {
		dev_err(dev, "skipping ufs eli read\n");
		ufs_vdi.eli = 0;
	} else if (hba->ufshcd_state == UFSHCD_STATE_OPERATIONAL) {
		pm_runtime_get_sync(hba->dev);
		ufs_sec_get_health_desc(hba);
		pm_runtime_put(hba->dev);
	} else {
		/* return previous ELI value if not operational */
		dev_info(hba->dev, "ufshcd_state: %d, old eli: %01x\n",
				hba->ufshcd_state, ufs_vdi.eli);
	}

	return sprintf(buf, "%u\n", ufs_vdi.eli);
}
static DEVICE_ATTR(eli, 0444, ufs_sec_eli_show, NULL);

static ssize_t ufs_sec_ic_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", ufs_vdi.ic);
}

static ssize_t ufs_sec_ic_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if (kstrtou32(buf, 0, &value))
		return -EINVAL;

	ufs_vdi.ic = value;

	return count;
}
static DEVICE_ATTR(ic, 0664, ufs_sec_ic_show, ufs_sec_ic_store);

static ssize_t ufs_sec_shi_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", ufs_vdi.shi);
}

static ssize_t ufs_sec_shi_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	char shi_buf[UFS_SHI_SIZE] = {0, };

	ret = sscanf(buf, "%255[^\n]%*c", shi_buf);

	if (ret != 1)
		return -EINVAL;

	snprintf(ufs_vdi.shi, UFS_SHI_SIZE, "%s", shi_buf);

	return count;
}
static DEVICE_ATTR(shi, 0664, ufs_sec_shi_show, ufs_sec_shi_store);

static ssize_t man_id_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ufs_hba *hba = dev_get_drvdata(dev);

	return sprintf(buf, "%04x\n", hba->dev_info.wmanufacturerid);
}
static DEVICE_ATTR_RO(man_id);

static bool ufs_sec_wait_for_clear_pending(struct ufs_hba *hba, u64 timeout_us)
{
	unsigned long flags;
	unsigned int tm_pending = 0;
	unsigned int tr_pending = 0;
	bool timeout = true;
	ktime_t start;

	ufshcd_hold(hba, false);

	start = ktime_get();

	do {
		spin_lock_irqsave(hba->host->host_lock, flags);

		tr_pending = 0;

		tm_pending = ufshcd_readl(hba, REG_UTP_TASK_REQ_DOOR_BELL);
		/* no mcq supports */
		tr_pending = ufshcd_readl(hba, REG_UTP_TRANSFER_REQ_DOOR_BELL);

		spin_unlock_irqrestore(hba->host->host_lock, flags);

		if (!tm_pending && !tr_pending) {
			dev_info(hba->dev, "doorbell clr complete.\n");
			timeout = false;
			break;
		}

		usleep_range(5000, 5100);
	} while (ktime_to_us(ktime_sub(ktime_get(), start)) < timeout_us);

	ufshcd_release(hba);

	return timeout;
}

static int ufs_sec_send_pon(struct ufs_hba *hba)
{
	struct scsi_device *sdp = hba->sdev_ufs_device;
	const unsigned char cdb[6] = { START_STOP, 0, 0, 0, UFS_POWERDOWN_PWR_MODE << 4, 0 };
	struct scsi_sense_hdr sshdr;
	int retries;
	int ret;

	for (retries = 3; retries > 0; --retries) {
		ret = __scsi_execute(sdp, cdb, DMA_NONE, NULL, 0, NULL, &sshdr,
				   10 * HZ, 0, 0, RQF_PM, NULL);
		if (ret <= 0)
			break;
	}

	if (ret) {
		if (ret > 0) {
			if (scsi_sense_valid(&sshdr))
				scsi_print_sense_hdr(sdp, NULL, &sshdr);
		}
	} else {
		dev_info(hba->dev, "pon done.\n");
		hba->curr_dev_pwr_mode = UFS_POWERDOWN_PWR_MODE;
	}

	return ret;
}

static void ufs_sec_reset_device(struct ufs_hba *hba)
{
	struct ufs_mtk_host *host = ufshcd_get_variant(hba);
	unsigned long flags;

	spin_lock_irqsave(hba->host->host_lock, flags);

	hba->force_reset = true;
	host->skip_flush = true;
	hba->ufshcd_state = UFSHCD_STATE_EH_SCHEDULED_FATAL;

	queue_work(hba->eh_wq, &hba->eh_work);

	spin_unlock_irqrestore(hba->host->host_lock, flags);

	flush_work(&hba->eh_work);

	dev_info(hba->dev, "reset done.\n");

	if (host->skip_flush)
		host->skip_flush = false;
}

static ssize_t ufs_sec_post_ffu_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct ufs_hba *hba = dev_get_drvdata(dev);
	struct scsi_device *sdp_wlu = hba->sdev_ufs_device;
	struct scsi_device *sdp;
	u32 ahit_backup = hba->ahit;
	unsigned long flags;
	int ret = 0;

#if IS_ENABLED(CONFIG_SCSI_UFS_TEST_MODE)
	dev_err(hba->dev, "post_ffu is not allowed if test mode is enabled\n");

	return -EINVAL;
#endif

	/* check product name string */
	if (strncmp(buf, (char *)hba->dev_info.model, strlen(hba->dev_info.model)))
		return -EINVAL;

	dev_info(hba->dev, "post_ffu start\n");

	pm_runtime_get_sync(hba->dev);

	spin_lock_irqsave(hba->host->host_lock, flags);

	if (sdp_wlu && scsi_device_online(sdp_wlu))
		ret = scsi_device_get(sdp_wlu);
	else
		ret = -ENODEV;

	spin_unlock_irqrestore(hba->host->host_lock, flags);

	if (ret)
		goto resume_rpm;

	/* set SDEV_QUIESCE */
	shost_for_each_device(sdp, hba->host)
		scsi_device_quiesce(sdp);

	/* wait for clear outstanding requests after queue quiesce */
	if (ufs_sec_wait_for_clear_pending(hba, USEC_PER_SEC)) {
		dev_err(dev, "post_ffu: doorbell clr timedout 1s.\n");
		ret = -ETIMEDOUT;
		goto resume_scsi_dev;
	}

	/* disable AH8 */
	ufshcd_auto_hibern8_update(hba, 0);

	ret = ufs_sec_send_pon(hba);
	if (ret) {
		/* if PON fails, do not reset UFS device */
		dev_err(dev, "post_ffu: pon failed.(%d)\n", ret);
		ret = -EBUSY;
	} else {
		/* reset UFS by eh_work */
		ufs_sec_reset_device(hba);
	}

	/* enable AH8 after UFS reset */
	ufshcd_auto_hibern8_update(hba, ahit_backup);

resume_scsi_dev:
	/* set SDEV_RUNNING */
	shost_for_each_device(sdp, hba->host)
		scsi_device_resume(sdp);

	scsi_device_put(sdp_wlu);

resume_rpm:
	pm_runtime_put(hba->dev);

	if (ret) {
		dev_err(hba->dev, "post_ffu error(%d).\n", ret);
		return ret;
	}

	dev_info(hba->dev, "post_ffu finish\n");

	return count;
}
static DEVICE_ATTR(post_ffu, 0220, NULL, ufs_sec_post_ffu_store);

/* SEC s_info : begin */
static ssize_t SEC_UFS_s_info_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	char s_buf[UFS_S_INFO_SIZE] = {0, };

	ret = sscanf(buf, "%511s", s_buf);

	if (ret != 1)
		return -EINVAL;

	snprintf(ufs_vdi.s_info, UFS_S_INFO_SIZE, "%s", s_buf);

	return count;
}

SEC_UFS_DATA_ATTR_RW(SEC_UFS_s_info, "%s\n", ufs_vdi.s_info);
/* SEC s_info : end */

/* UFS SEC WB : begin */
static ssize_t ufs_sec_wb_support_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ufs_hba *hba = dev_get_drvdata(dev);

	return sprintf(buf, "%s:%s\n", ufs_wb.support ? "Support" : "No support",
			hba->wb_enabled ? "on" : "off");
}
static DEVICE_ATTR(sec_wb_support, 0444, ufs_sec_wb_support_show, NULL);

static ssize_t ufs_sec_wb_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u32 value;
	struct ufs_hba *hba = dev_get_drvdata(dev);
	unsigned long flags;

	if (!ufs_wb.setup_done) {
		dev_err(hba->dev, "SEC WB is not ready yet.\n");
		return -ENODEV;
	}

	if (!ufs_sec_is_wb_allowed()) {
		pr_err("%s: not allowed.\n", __func__);
		return -EPERM;
	}

	if (kstrtou32(buf, 0, &value))
		return -EINVAL;

	spin_lock_irqsave(hba->host->host_lock, flags);
	value = !!value;

	if (!value) {
		if (atomic_inc_return(&ufs_wb.wb_off_cnt) == 1) {
			ufs_wb.wb_off = true;
			pr_err("disable SEC WB : state %d.\n", ufs_wb.state);
		}
	} else {
		if (atomic_dec_and_test(&ufs_wb.wb_off_cnt)) {
			ufs_wb.wb_off = false;
			pr_err("enable SEC WB.\n");
		}
	}
	spin_unlock_irqrestore(hba->host->host_lock, flags);

	return count;
}

static ssize_t ufs_sec_wb_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", ufs_wb.wb_off ? "off" : "Enabled");
}
static DEVICE_ATTR(sec_wb_enable, 0664, ufs_sec_wb_enable_show, ufs_sec_wb_enable_store);

SEC_UFS_WB_DATA_ATTR(wb_up_threshold_block, "%d\n", up_threshold_block);
SEC_UFS_WB_DATA_ATTR(wb_up_threshold_rqs, "%d\n", up_threshold_rqs);
SEC_UFS_WB_DATA_ATTR(wb_down_threshold_block, "%d\n", down_threshold_block);
SEC_UFS_WB_DATA_ATTR(wb_down_threshold_rqs, "%d\n", down_threshold_rqs);
SEC_UFS_WB_DATA_ATTR(lp_wb_up_threshold_block, "%d\n", lp_up_threshold_block);
SEC_UFS_WB_DATA_ATTR(lp_wb_up_threshold_rqs, "%d\n", lp_up_threshold_rqs);
SEC_UFS_WB_DATA_ATTR(lp_wb_down_threshold_block, "%d\n", lp_down_threshold_block);
SEC_UFS_WB_DATA_ATTR(lp_wb_down_threshold_rqs, "%d\n", lp_down_threshold_rqs);

SEC_UFS_WB_TIME_ATTR(wb_on_delay_ms, "%d\n", on_delay);
SEC_UFS_WB_TIME_ATTR(wb_off_delay_ms, "%d\n", off_delay);
SEC_UFS_WB_TIME_ATTR(lp_wb_on_delay_ms, "%d\n", lp_on_delay);
SEC_UFS_WB_TIME_ATTR(lp_wb_off_delay_ms, "%d\n", lp_off_delay);

SEC_UFS_WB_DATA_RO_ATTR(wb_state, "%d,%u\n", ufs_wb.state, jiffies_to_msecs(jiffies - ufs_wb.state_ts));
SEC_UFS_WB_DATA_RO_ATTR(wb_current_stat, "current : block %d, rqs %d, issued blocks %d\n",
		ufs_wb.current_block, ufs_wb.current_rqs, ufs_wb.curr_issued_block);
SEC_UFS_WB_DATA_RO_ATTR(wb_current_min_max_stat, "current issued blocks : min %d, max %d.\n",
		(ufs_wb.curr_issued_min_block == INT_MAX) ? 0 : ufs_wb.curr_issued_min_block,
		ufs_wb.curr_issued_max_block);
SEC_UFS_WB_DATA_RO_ATTR(wb_total_stat, "total : %dMB\n\t<  4GB:%d\n\t<  8GB:%d\n\t< 16GB:%d\n\t>=16GB:%d\n",
		ufs_wb.total_issued_mb,
		ufs_wb.issued_size_cnt[0],
		ufs_wb.issued_size_cnt[1],
		ufs_wb.issued_size_cnt[2],
		ufs_wb.issued_size_cnt[3]);

static ssize_t SEC_UFS_op_cnt_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	if ((buf[0] != 'C' && buf[0] != 'c') || (count != 1))
		return -EINVAL;

	SEC_UFS_ERR_INFO_BACKUP(op_count, HW_RESET_count);
	SEC_UFS_ERR_INFO_BACKUP(op_count, link_startup_count);
	SEC_UFS_ERR_INFO_BACKUP(op_count, Hibern8_enter_count);
	SEC_UFS_ERR_INFO_BACKUP(op_count, Hibern8_exit_count);

	return count;
}

static ssize_t SEC_UFS_uic_cmd_cnt_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	if ((buf[0] != 'C' && buf[0] != 'c') || (count != 1))
		return -EINVAL;

	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_TEST_MODE_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_GET_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_SET_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_PEER_GET_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_PEER_SET_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_POWERON_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_POWEROFF_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_ENABLE_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_RESET_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_END_PT_RST_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_LINK_STARTUP_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_HIBER_ENTER_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, DME_HIBER_EXIT_err);

	return count;
}

static ssize_t SEC_UFS_uic_err_cnt_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	if ((buf[0] != 'C' && buf[0] != 'c') || (count != 1))
		return -EINVAL;

	SEC_UFS_ERR_INFO_BACKUP(UIC_err_count, PA_ERR_cnt);
	SEC_UFS_ERR_INFO_BACKUP(UIC_err_count, DL_PA_INIT_ERROR_cnt);
	SEC_UFS_ERR_INFO_BACKUP(UIC_err_count, DL_NAC_RECEIVED_ERROR_cnt);
	SEC_UFS_ERR_INFO_BACKUP(UIC_err_count, DL_TC_REPLAY_ERROR_cnt);
	SEC_UFS_ERR_INFO_BACKUP(UIC_err_count, NL_ERROR_cnt);
	SEC_UFS_ERR_INFO_BACKUP(UIC_err_count, TL_ERROR_cnt);
	SEC_UFS_ERR_INFO_BACKUP(UIC_err_count, DME_ERROR_cnt);

	return count;
}

static ssize_t SEC_UFS_fatal_cnt_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	if ((buf[0] != 'C' && buf[0] != 'c') || (count != 1))
		return -EINVAL;

	SEC_UFS_ERR_INFO_BACKUP(Fatal_err_count, DFE);
	SEC_UFS_ERR_INFO_BACKUP(Fatal_err_count, CFE);
	SEC_UFS_ERR_INFO_BACKUP(Fatal_err_count, SBFE);
	SEC_UFS_ERR_INFO_BACKUP(Fatal_err_count, CEFE);
	SEC_UFS_ERR_INFO_BACKUP(Fatal_err_count, LLE);

	return count;
}

static ssize_t SEC_UFS_utp_cnt_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	if ((buf[0] != 'C' && buf[0] != 'c') || (count != 1))
		return -EINVAL;

	SEC_UFS_ERR_INFO_BACKUP(UTP_count, UTMR_query_task_count);
	SEC_UFS_ERR_INFO_BACKUP(UTP_count, UTMR_abort_task_count);
	SEC_UFS_ERR_INFO_BACKUP(UTP_count, UTR_read_err);
	SEC_UFS_ERR_INFO_BACKUP(UTP_count, UTR_write_err);
	SEC_UFS_ERR_INFO_BACKUP(UTP_count, UTR_sync_cache_err);
	SEC_UFS_ERR_INFO_BACKUP(UTP_count, UTR_unmap_err);
	SEC_UFS_ERR_INFO_BACKUP(UTP_count, UTR_etc_err);

	return count;
}

static ssize_t SEC_UFS_query_cnt_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	if ((buf[0] != 'C' && buf[0] != 'c') || (count != 1))
		return -EINVAL;

	SEC_UFS_ERR_INFO_BACKUP(query_count, NOP_err);
	SEC_UFS_ERR_INFO_BACKUP(query_count, R_Desc_err);
	SEC_UFS_ERR_INFO_BACKUP(query_count, W_Desc_err);
	SEC_UFS_ERR_INFO_BACKUP(query_count, R_Attr_err);
	SEC_UFS_ERR_INFO_BACKUP(query_count, W_Attr_err);
	SEC_UFS_ERR_INFO_BACKUP(query_count, R_Flag_err);
	SEC_UFS_ERR_INFO_BACKUP(query_count, Set_Flag_err);
	SEC_UFS_ERR_INFO_BACKUP(query_count, Clear_Flag_err);
	SEC_UFS_ERR_INFO_BACKUP(query_count, Toggle_Flag_err);

	return count;
}

static ssize_t SEC_UFS_err_sum_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	if ((buf[0] != 'C' && buf[0] != 'c') || (count != 1))
		return -EINVAL;

	SEC_UFS_ERR_INFO_BACKUP(op_count, op_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_cmd_count, UIC_cmd_err);
	SEC_UFS_ERR_INFO_BACKUP(UIC_err_count, UIC_err);
	SEC_UFS_ERR_INFO_BACKUP(Fatal_err_count, Fatal_err);
	SEC_UFS_ERR_INFO_BACKUP(UTP_count, UTP_err);
	SEC_UFS_ERR_INFO_BACKUP(query_count, Query_err);

	return count;
}

static ssize_t sense_err_count_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	if ((buf[0] != 'C' && buf[0] != 'c') || (count != 1))
		return -EINVAL;

	SEC_UFS_ERR_INFO_BACKUP(sense_count, scsi_medium_err);
	SEC_UFS_ERR_INFO_BACKUP(sense_count, scsi_hw_err);

	return count;
}

SEC_UFS_DATA_ATTR_RW(SEC_UFS_op_cnt, "\"HWRESET\":\"%u\",\"LINKFAIL\":\"%u\",\"H8ENTERFAIL\":\"%u\""
		",\"H8EXITFAIL\":\"%u\"\n",
		ufs_err_info.op_count.HW_RESET_count,
		ufs_err_info.op_count.link_startup_count,
		ufs_err_info.op_count.Hibern8_enter_count,
		ufs_err_info.op_count.Hibern8_exit_count);

SEC_UFS_DATA_ATTR_RW(SEC_UFS_uic_cmd_cnt, "\"TESTMODE\":\"%u\",\"DME_GET\":\"%u\",\"DME_SET\":\"%u\""
		",\"DME_PGET\":\"%u\",\"DME_PSET\":\"%u\",\"PWRON\":\"%u\",\"PWROFF\":\"%u\""
		",\"DME_EN\":\"%u\",\"DME_RST\":\"%u\",\"EPRST\":\"%u\",\"LINKSTARTUP\":\"%u\""
		",\"H8ENTER\":\"%u\",\"H8EXIT\":\"%u\"\n",
		ufs_err_info.UIC_cmd_count.DME_TEST_MODE_err,		// TEST_MODE
		ufs_err_info.UIC_cmd_count.DME_GET_err,			// DME_GET
		ufs_err_info.UIC_cmd_count.DME_SET_err,			// DME_SET
		ufs_err_info.UIC_cmd_count.DME_PEER_GET_err,		// DME_PEER_GET
		ufs_err_info.UIC_cmd_count.DME_PEER_SET_err,		// DME_PEER_SET
		ufs_err_info.UIC_cmd_count.DME_POWERON_err,		// DME_POWERON
		ufs_err_info.UIC_cmd_count.DME_POWEROFF_err,		// DME_POWEROFF
		ufs_err_info.UIC_cmd_count.DME_ENABLE_err,		// DME_ENABLE
		ufs_err_info.UIC_cmd_count.DME_RESET_err,		// DME_RESET
		ufs_err_info.UIC_cmd_count.DME_END_PT_RST_err,		// DME_END_PT_RST
		ufs_err_info.UIC_cmd_count.DME_LINK_STARTUP_err,	// DME_LINK_STARTUP
		ufs_err_info.UIC_cmd_count.DME_HIBER_ENTER_err,		// DME_HIBERN8_ENTER
		ufs_err_info.UIC_cmd_count.DME_HIBER_EXIT_err);		// DME_HIBERN8_EXIT

SEC_UFS_DATA_ATTR_RW(SEC_UFS_uic_err_cnt, "\"PAERR\":\"%u\",\"DLPAINITERROR\":\"%u\",\"DLNAC\":\"%u\""
		",\"DLTCREPLAY\":\"%u\",\"NLERR\":\"%u\",\"TLERR\":\"%u\",\"DMEERR\":\"%u\"\n",
		ufs_err_info.UIC_err_count.PA_ERR_cnt,			// PA_ERR
		ufs_err_info.UIC_err_count.DL_PA_INIT_ERROR_cnt,	// DL_PA_INIT_ERROR
		ufs_err_info.UIC_err_count.DL_NAC_RECEIVED_ERROR_cnt,	// DL_NAC_RECEIVED
		ufs_err_info.UIC_err_count.DL_TC_REPLAY_ERROR_cnt,	// DL_TCx_REPLAY_ERROR
		ufs_err_info.UIC_err_count.NL_ERROR_cnt,		// NL_ERROR
		ufs_err_info.UIC_err_count.TL_ERROR_cnt,		// TL_ERROR
		ufs_err_info.UIC_err_count.DME_ERROR_cnt);		// DME_ERROR

SEC_UFS_DATA_ATTR_RW(SEC_UFS_fatal_cnt, "\"DFE\":\"%u\",\"CFE\":\"%u\",\"SBFE\":\"%u\""
		",\"CEFE\":\"%u\",\"LLE\":\"%u\"\n",
		ufs_err_info.Fatal_err_count.DFE,		// Device_Fatal
		ufs_err_info.Fatal_err_count.CFE,		// Controller_Fatal
		ufs_err_info.Fatal_err_count.SBFE,		// System_Bus_Fatal
		ufs_err_info.Fatal_err_count.CEFE,		// Crypto_Engine_Fatal
		ufs_err_info.Fatal_err_count.LLE);		// Link_Lost

SEC_UFS_DATA_ATTR_RW(SEC_UFS_utp_cnt, "\"UTMRQTASK\":\"%u\",\"UTMRATASK\":\"%u\",\"UTRR\":\"%u\""
		",\"UTRW\":\"%u\",\"UTRSYNCCACHE\":\"%u\",\"UTRUNMAP\":\"%u\",\"UTRETC\":\"%u\"\n",
		ufs_err_info.UTP_count.UTMR_query_task_count,	// QUERY_TASK
		ufs_err_info.UTP_count.UTMR_abort_task_count,	// ABORT_TASK
		ufs_err_info.UTP_count.UTR_read_err,		// READ_10
		ufs_err_info.UTP_count.UTR_write_err,		// WRITE_10
		ufs_err_info.UTP_count.UTR_sync_cache_err,	// SYNC_CACHE
		ufs_err_info.UTP_count.UTR_unmap_err,		// UNMAP
		ufs_err_info.UTP_count.UTR_etc_err);		// etc

SEC_UFS_DATA_ATTR_RW(SEC_UFS_query_cnt, "\"NOPERR\":\"%u\",\"R_DESC\":\"%u\",\"W_DESC\":\"%u\""
		",\"R_ATTR\":\"%u\",\"W_ATTR\":\"%u\",\"R_FLAG\":\"%u\",\"S_FLAG\":\"%u\""
		",\"C_FLAG\":\"%u\",\"T_FLAG\":\"%u\"\n",
		ufs_err_info.query_count.NOP_err,
		ufs_err_info.query_count.R_Desc_err,		// R_Desc
		ufs_err_info.query_count.W_Desc_err,		// W_Desc
		ufs_err_info.query_count.R_Attr_err,		// R_Attr
		ufs_err_info.query_count.W_Attr_err,		// W_Attr
		ufs_err_info.query_count.R_Flag_err,		// R_Flag
		ufs_err_info.query_count.Set_Flag_err,		// Set_Flag
		ufs_err_info.query_count.Clear_Flag_err,	// Clear_Flag
		ufs_err_info.query_count.Toggle_Flag_err);	// Toggle_Flag

SEC_UFS_DATA_ATTR_RW(sense_err_count, "\"MEDIUM\":\"%u\",\"HWERR\":\"%u\"\n",
		ufs_err_info.sense_count.scsi_medium_err,
		ufs_err_info.sense_count.scsi_hw_err);

/* daily err sum */
SEC_UFS_DATA_ATTR_RW(SEC_UFS_err_sum, "\"OPERR\":\"%u\",\"UICCMD\":\"%u\",\"UICERR\":\"%u\""
		",\"FATALERR\":\"%u\",\"UTPERR\":\"%u\",\"QUERYERR\":\"%u\"\n",
		ufs_err_info.op_count.op_err,
		ufs_err_info.UIC_cmd_count.UIC_cmd_err,
		ufs_err_info.UIC_err_count.UIC_err,
		ufs_err_info.Fatal_err_count.Fatal_err,
		ufs_err_info.UTP_count.UTP_err,
		ufs_err_info.query_count.Query_err);

/* accumulated err sum */
SEC_UFS_DATA_ATTR_RO(SEC_UFS_err_summary,
		"OPERR : %u, UICCMD : %u, UICERR : %u, FATALERR : %u, UTPERR : %u, QUERYERR : %u\n"
		"MEDIUM : %u, HWERR : %u\n",
		SEC_UFS_ERR_INFO_GET_VALUE(op_count, op_err),
		SEC_UFS_ERR_INFO_GET_VALUE(UIC_cmd_count, UIC_cmd_err),
		SEC_UFS_ERR_INFO_GET_VALUE(UIC_err_count, UIC_err),
		SEC_UFS_ERR_INFO_GET_VALUE(Fatal_err_count, Fatal_err),
		SEC_UFS_ERR_INFO_GET_VALUE(UTP_count, UTP_err),
		SEC_UFS_ERR_INFO_GET_VALUE(query_count, Query_err),
		SEC_UFS_ERR_INFO_GET_VALUE(sense_count, scsi_medium_err),
		SEC_UFS_ERR_INFO_GET_VALUE(sense_count, scsi_hw_err));

SEC_UFS_DATA_ATTR_RO(sense_err_logging, "\"LBA0\":\"%lx\",\"LBA1\":\"%lx\",\"LBA2\":\"%lx\""
		",\"LBA3\":\"%lx\",\"LBA4\":\"%lx\",\"LBA5\":\"%lx\""
		",\"LBA6\":\"%lx\",\"LBA7\":\"%lx\",\"LBA8\":\"%lx\",\"LBA9\":\"%lx\""
		",\"REGIONMAP\":\"%016llx\"\n",
		ufs_err_info.sense_err_log.issue_LBA_list[0],
		ufs_err_info.sense_err_log.issue_LBA_list[1],
		ufs_err_info.sense_err_log.issue_LBA_list[2],
		ufs_err_info.sense_err_log.issue_LBA_list[3],
		ufs_err_info.sense_err_log.issue_LBA_list[4],
		ufs_err_info.sense_err_log.issue_LBA_list[5],
		ufs_err_info.sense_err_log.issue_LBA_list[6],
		ufs_err_info.sense_err_log.issue_LBA_list[7],
		ufs_err_info.sense_err_log.issue_LBA_list[8],
		ufs_err_info.sense_err_log.issue_LBA_list[9],
		ufs_err_info.sense_err_log.issue_region_map);

/* SEC cmd log : begin */
static ssize_t ufs_sec_cmd_log_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ufs_sec_cmd_log_info *ufs_cmd_log =
		ufs_sec_features.ufs_cmd_log;
	struct ufs_sec_cmd_log_entry *entry = NULL;
	int i = (ufs_cmd_log->pos + UFS_SEC_CMD_LOGGING_MAX
			- UFS_SEC_CMD_LOGNODE_MAX);
	int idx = 0;
	int len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len,
			"%2s: %10s: %2s %3s %4s %9s %6s %16s\n",
			"No", "log string", "lu", "tag",
			"c_id", "lba", "length", "time");

	for (idx = 0; idx < UFS_SEC_CMD_LOGNODE_MAX; idx++, i++) {
		i %= UFS_SEC_CMD_LOGGING_MAX;
		entry = &ufs_cmd_log->entries[i];
		len += snprintf(buf + len, PAGE_SIZE - len,
				"%2d: %10s: %2d %3d 0x%02x %9u %6d %16llu\n",
				idx,
				entry->str, entry->lun, entry->tag,
				entry->cmd_id, entry->lba,
				entry->transfer_len, entry->tstamp);
	}

	return len;
}
static DEVICE_ATTR(cmd_log, 0440, ufs_sec_cmd_log_show, NULL);

static struct attribute *sec_ufs_info_attributes[] = {
	&dev_attr_un.attr,
	&dev_attr_lt.attr,
	&dev_attr_flt.attr,
	&dev_attr_eli.attr,
	&dev_attr_ic.attr,
	&dev_attr_shi.attr,
	&dev_attr_man_id.attr,
	&dev_attr_post_ffu.attr,
	NULL
};

static struct attribute *sec_ufs_error_attributes[] = {
	&dev_attr_SEC_UFS_op_cnt.attr,
	&dev_attr_SEC_UFS_uic_cmd_cnt.attr,
	&dev_attr_SEC_UFS_uic_err_cnt.attr,
	&dev_attr_SEC_UFS_fatal_cnt.attr,
	&dev_attr_SEC_UFS_utp_cnt.attr,
	&dev_attr_SEC_UFS_query_cnt.attr,
	&dev_attr_SEC_UFS_err_sum.attr,
	&dev_attr_sense_err_count.attr,
	&dev_attr_sense_err_logging.attr,
	&dev_attr_SEC_UFS_err_summary.attr,
	&dev_attr_SEC_UFS_s_info.attr,
	NULL
};

static struct attribute *sec_ufs_wb_attributes[] = {
	&dev_attr_sec_wb_support.attr,
	&dev_attr_sec_wb_enable.attr,
	&dev_attr_wb_up_threshold_block.attr,
	&dev_attr_wb_up_threshold_rqs.attr,
	&dev_attr_wb_down_threshold_block.attr,
	&dev_attr_wb_down_threshold_rqs.attr,
	&dev_attr_lp_wb_up_threshold_block.attr,
	&dev_attr_lp_wb_up_threshold_rqs.attr,
	&dev_attr_lp_wb_down_threshold_block.attr,
	&dev_attr_lp_wb_down_threshold_rqs.attr,
	&dev_attr_wb_on_delay_ms.attr,
	&dev_attr_wb_off_delay_ms.attr,
	&dev_attr_lp_wb_on_delay_ms.attr,
	&dev_attr_lp_wb_off_delay_ms.attr,
	&dev_attr_wb_state.attr,
	&dev_attr_wb_current_stat.attr,
	&dev_attr_wb_current_min_max_stat.attr,
	&dev_attr_wb_total_stat.attr,
	NULL
};

static struct attribute *sec_ufs_cmd_log_attributes[] = {
	&dev_attr_cmd_log.attr,
	NULL
};

static struct attribute_group sec_ufs_wb_attribute_group = {
	.attrs	= sec_ufs_wb_attributes,
};

static struct attribute_group sec_ufs_info_attribute_group = {
	.attrs	= sec_ufs_info_attributes,
};

static struct attribute_group sec_ufs_error_attribute_group = {
	.attrs	= sec_ufs_error_attributes,
};

static struct attribute_group sec_ufs_cmd_log_attribute_group = {
	.attrs	= sec_ufs_cmd_log_attributes,
};

void ufs_sec_create_info_sysfs(struct ufs_hba *hba)
{
	int ret = 0;

	ret = sysfs_create_group(&sec_ufs_cmd_dev->kobj,
			&sec_ufs_info_attribute_group);
	if (ret)
		dev_err(hba->dev, "%s: Failed to create sec_ufs_info sysfs group, %d\n",
				__func__, ret);
}

void ufs_sec_create_err_sysfs(struct ufs_hba *hba)
{
	int ret = 0;
	struct device *dev = &(hba->host->shost_dev);

	/* scsi_host sysfs nodes */
	ret = sysfs_create_group(&dev->kobj, &sec_ufs_error_attribute_group);
	if (ret)
		pr_err("cannot create sec error sysfs group err: %d\n", ret);
}

void ufs_sec_create_wb_sysfs(struct ufs_hba *hba)
{
	int ret = 0;

	if (!ufs_wb.setup_done)
		return;

	ret = sysfs_create_group(&sec_ufs_cmd_dev->kobj,
			&sec_ufs_wb_attribute_group);
	if (ret)
		dev_err(hba->dev, "%s: Failed to create sec_ufs_wb sysfs group, %d\n",
				__func__, ret);
}

void ufs_sec_create_cmd_log_sysfs(struct ufs_hba *hba)
{
	int ret = 0;

	ret = sysfs_create_group(&sec_ufs_cmd_dev->kobj,
			&sec_ufs_cmd_log_attribute_group);
	if (ret)
		dev_err(hba->dev, "%s: Failed to create sec_ufs_cmd_log sysfs group, %d\n",
				__func__, ret);
}

static int ufs_sec_create_sysfs_dev(struct ufs_hba *hba)
{
	/* sec specific vendor sysfs nodes */
	if (!sec_ufs_cmd_dev)
		sec_ufs_cmd_dev = sec_device_create(hba, "ufs");

	if (IS_ERR(sec_ufs_cmd_dev)) {
		pr_err("Fail to create sysfs dev\n");
		return -ENODEV;
	}

	return 0;
}

void ufs_sec_add_sysfs_nodes(struct ufs_hba *hba)
{
	ufs_sec_create_err_sysfs(hba);

	if (!ufs_sec_create_sysfs_dev(hba)) {
		/* sec specific vendor sysfs nodes */
		ufs_sec_create_info_sysfs(hba);

		ufs_sec_create_wb_sysfs(hba);

		if (ufs_sec_features.ufs_cmd_log)
			ufs_sec_create_cmd_log_sysfs(hba);
	}
}

void ufs_sec_remove_sysfs_nodes(struct ufs_hba *hba)
{
	sysfs_remove_group(&sec_ufs_cmd_dev->kobj, &sec_ufs_info_attribute_group);
}

MODULE_LICENSE("GPL v2");
