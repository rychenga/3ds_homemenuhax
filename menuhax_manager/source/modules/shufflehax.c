#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <3ds.h>

#include "archive.h"
#include "log.h"

#include "modules_common.h"

Result shufflehax_install(char *menuhax_basefn, s16 menuversion);
Result shufflehax_delete();

void register_module_shufflehax()
{
	module_entry module = {
		.name = "shufflehax",
		.haxinstall = shufflehax_install,
		.haxdelete = shufflehax_delete,
		.themeflag = false
	};

	register_module(&module);
}

Result shufflehax_install(char *menuhax_basefn, s16 menuversion)
{
	Result ret=0;

	char payload_filepath[256];
	char tmpstr[256];

	memset(payload_filepath, 0, sizeof(payload_filepath));
	memset(tmpstr, 0, sizeof(tmpstr));

	snprintf(payload_filepath, sizeof(payload_filepath)-1, "romfs:/finaloutput/stage1_themedata.zip@%s.bin", menuhax_basefn);
	snprintf(tmpstr, sizeof(tmpstr)-1, "sdmc:/menuhax/stage1/%s.bin", menuhax_basefn);

	log_printf(LOGTAR_ALL, "Copying stage1 to SD...\n");
	log_printf(LOGTAR_LOG, "Src path = '%s', dst = '%s'.\n", payload_filepath, tmpstr);

	unlink(tmpstr);
	ret = archive_copyfile(SDArchive, SDArchive, payload_filepath, tmpstr, filebuffer, 0, 0x1000, 0, "stage1");
	if(ret!=0)
	{
		log_printf(LOGTAR_ALL, "Failed to copy stage1 to SD: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	memset(payload_filepath, 0, sizeof(payload_filepath));

	snprintf(payload_filepath, sizeof(payload_filepath)-1, "romfs:/finaloutput/shufflepayload.zip@%s.lz", menuhax_basefn);

	log_printf(LOGTAR_ALL, "Enabling shuffle themecache...\n");
	ret = enablethemecache(3, 1, 1);
	if(ret!=0)return ret;

	log_printf(LOGTAR_ALL, "Installing to the SD theme-cache...\n");
	ret = sd2themecache(payload_filepath, "sdmc:/3ds/menuhax_manager/bgm_bundledmenuhax.bcstm", 0);
	if(ret!=0)return ret;

	log_printf(LOGTAR_ALL, "Initializing the seperate menuhax theme-data files...\n");
	ret = sd2themecache("romfs:/blanktheme.lz", NULL, 1);
	if(ret!=0)return ret;

	return 0;
}

Result shufflehax_delete()
{
	Result ret=0;
	unsigned int i;
	u8 *tmpbuf;

	char str[256];

	log_printf(LOGTAR_ALL, "Disabling theme-usage via SaveData.dat...\n");
	ret = disablethemecache();
	if(ret!=0)return ret;

	memset(filebuffer, 0, filebuffer_maxsize);

	tmpbuf = malloc(0xd20000);
	if(tmpbuf==NULL)
	{
		log_printf(LOGTAR_ALL, "Failed to allocate memory for the shuffle-BodyCache clearing buffer.");
		return 1;
	}
	memset(tmpbuf, 0, 0xd20000);

	log_printf(LOGTAR_ALL, "Clearing the theme-cache extdata now...\n");

	log_printf(LOGTAR_ALL, "Clearing the ThemeManage...\n");
	ret = archive_writefile(Theme_Extdata, "/ThemeManage.bin", filebuffer, 0x800, 0x800);
	if(ret!=0)
	{
		log_printf(LOGTAR_ALL, "Failed to clear the ThemeManage: 0x%08x.\n", (unsigned int)ret);
		free(tmpbuf);
		return ret;
	}

	log_printf(LOGTAR_ALL, "Clearing the shuffle BodyCache...\n");
	ret = archive_writefile(Theme_Extdata, "/BodyCache_rd.bin", tmpbuf, 0xd20000, 0xd20000);
	free(tmpbuf);
	if(ret!=0)
	{
		log_printf(LOGTAR_ALL, "Failed to clear the shuffle BodyCache: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	for(i=0; i<10; i++)
	{
		memset(str, 0, sizeof(str));
		snprintf(str, sizeof(str)-1, "/BgmCache_%02d.bin", i);

		log_printf(LOGTAR_ALL, "Clearing shuffle BgmCache_%02d...\n", i);
		ret = archive_writefile(Theme_Extdata, str, filebuffer, 0x337000, 0x337000);
		if(ret!=0)
		{
			log_printf(LOGTAR_ALL, "Failed to clear shuffle BgmCache_%02d: 0x%08x.\n", i, (unsigned int)ret);
			return ret;
		}
	}

	return 0;
}

