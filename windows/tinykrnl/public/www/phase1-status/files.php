<?php

/* TinyKrnl Project Status - Phase 1 Overview - Status Script
 * File - files.php
 * Written by Peter Johansson. */

/* This file grabs the finished procentage from the SVN modules.txt data into a variable 
 */

/* Note that the variable called $module_full_data is for the 100% check
 * Due the precentage in the modulestatus.txt files starts from 0 example 010 which is 10%
 * When we are going to compare the procentage and add a organge block for every + 10% 
 * we then cant let the procentage begin with 0, Example 01 is greater than 02 when we are calculating negative digits.
 * That's why we let preg_match just grab the numbers before 0, example 010 will be 10, Problem solved almost ;)
 * Now we will run into a problem when module.txt reaches 100, because we are just grabbing the two last numbers
 * Hence 100 will become 00, That's why we letting $module_full_data only grab the full digits output.
 *
 * And this is a very good solution, i mean continue generate the modulestatus.txt files with 3 digits
 * If we would just let the digits under 100% be written out with 2 digits, example 20 instead of 020.
 * Then we would run into Bigger problems when the module.txt becomes 100%, 
 * because then it would only get 00 instead of 100, and it would become a pain to get preg_match to grab 3 digits
 * only when status.txt is 100%. So well just continue the way you are doing right now, it's much flexible and friendly.
 *
 * Regards
 * Peter Joahnsson
 */

/* NOTE
 * Your php.ini need's to allow PHP to fopen to open files trough HTTP
 * Inorder to do that change the following line in php.ini
 * allow_url_fopen = OFF to allow_url_fopen = On
 */

/* Add following FLAGS to the module status.txt files inorder to check Boots,Works,Mature
 * Example...
 * 
 * You can have a look at status.txt, it checks if boots,works, or mature is == 1
 * if either boots,works, or mature is == 1 Then it will add the extra boxes to the status page
 * You only need to define any of these as 1 if they work, because if not any of those is == 1 
 * it wont display the extra boxes
 * Boots: 1
 * Works: 1
 * Mature: 1
 */

/* ATAPI */ 														
$file = "http://svn.tinykrnl.org/svn/tinykrnl/drivers/storage/ide/atapi/atstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$atapi = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$atapi_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$atapi_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$atapi_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$atapi_mature = "{$matches[1]}";

/* BootVid */  	  													 
$file = "http://svn.tinykrnl.org/svn/tinykrnl/base/boot/bootvid/bvstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$bootvid = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$bootvid_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$bootvid_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$bootvid_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$bootvid_mature = "{$matches[1]}";


/* ClassPnP */  	  											  	 
$file = "http://svn.tinykrnl.org/svn/tinykrnl/drivers/storage/classpnp/cpstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$classpnp = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$classpnp_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$classpnp_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$classpnp_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$classpnp_mature = "{$matches[1]}";

/* Disk */  	  											  	  	 
$file = "http://svn.tinykrnl.org/svn/tinykrnl/drivers/storage/disk/dkstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$disk = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$disk_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$disk_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$disk_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$disk_mature = "{$matches[1]}";

/* FastFAT */  	  											  	  	 
/* ADD STATUS TXT TO $file ="" */
$file = "";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$fastfat = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$fastfat_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$fastfat_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$fastfat_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$fastfat_mature = "{$matches[1]}";

/* FtDisk */  	  											  	  	 
$file = "http://svn.tinykrnl.org/svn/tinykrnl/drivers/storage/newft/ftstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$ftdisk = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$ftdisk_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$ftdisk_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$ftdisk_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$ftdisk_mature = "{$matches[1]}";

/* i8042prt */  	  											  	  
/* ADD STATUS TXT TO $file ="" */
$file = "";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$i8042prt = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$i8042prt_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$i8042prt_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$i8042prt_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$i8042prt_mature = "{$matches[1]}";

/* ISAPnP */   			  	  	  	  	  	  	  			 
$file = "http://svn.tinykrnl.org/svn/tinykrnl/base/busdrv/isapnp/isstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$isapnp = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$isapnp_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$isapnp_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$isapnp_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$isapnp_mature = "{$matches[1]}";

/* IntelIDE */										 
/* ADD STATUS TXT TO $file ="" */
$file = "";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$intelide = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$intelide_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$intelide_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$intelide_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$intelide_mature = "{$matches[1]}";

/* KbdClass */ 	  										  	  	  	 
$file = "http://svn.tinykrnl.org/svn/tinykrnl/drivers/input/kbdclass/kbstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$kbdclass = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$kbdclass_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$kbdclass_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$kbdclass_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$kbdclass_mature = "{$matches[1]}";

/* KDCom */  													 
$file = "http://svn.tinykrnl.org/svn/tinykrnl/base/boot/kdcom/kdstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$kdcom = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$kdcom_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$kdcom_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$kdcom_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$kdcom_mature = "{$matches[1]}";

/* MountMgr */  													 
$file = "http://svn.tinykrnl.org/svn/tinykrnl/drivers/filters/mountmgr/mnstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$mountmgr = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$mountmgr_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$mountmgr_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$mountmgr_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$mountmgr_mature = "{$matches[1]}";

/* PartMgr */  													
$file = "http://svn.tinykrnl.org/svn/tinykrnl/drivers/storage/partmgr/pmstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$partmgr = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$partmgr_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$partmgr_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$partmgr_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$partmgr_mature = "{$matches[1]}";

/* PCI */  											  	  	 
/* ADD STATUS TXT TO $file ="" */
$file = "";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$pci = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$pci_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$pci_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$pci_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$pci_mature = "{$matches[1]}";

/* PCIIDEx */  											  	  	 
$file = "http://svn.tinykrnl.org/svn/tinykrnl/drivers/storage/ide/pciidex/pxstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$pciidex = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$pciidex_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$pciidex_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$pciidex_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$pciidex_mature = "{$matches[1]}";

/* WMILib */  													
/* ADD STATUS TXT TO $file ="" */
$file = "";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$wmilib = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$wmilib_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$wmilib_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$wmilib_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$awmilib_mature = "{$matches[1]}";

/* HAL */  			  	  	  	  	  	  	  	  	  	  	 
/* ADD STATUS TXT TO $file ="" */
$file = "";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$hal = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$hal_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$hal_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$hal_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$hal_mature = "{$matches[1]}";

/* Kernel */  			  	  	  	  	  	  	  	  	  	  	 
/* ADD STATUS TXT TO $file ="" */
$file = "";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$kernel = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$kernel_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$kernel_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$kernel_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$kernel_mature = "{$matches[1]}";

/* UM Sys.DLL */						  	  	  	  	  	  	  	  
$file = "http://svn.tinykrnl.org/svn/tinykrnl/base/ntdll/ntstatus.txt?view=co";
$lines = file_get_contents($file);

/* Finished Precentage */
preg_match("/% COMPLETED: \d(\d\d)/",$lines,$matches); 
$umsysdll = "{$matches[1]}";
preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$unsysdll_full_data = "{$matches[1]}";

/* Boots,Works,Mature Flags */
preg_match("/Boots: (\d)/",$lines,$matches); 
$umsysdll_boots = "{$matches[1]}";
preg_match("/Works: (\d)/",$lines,$matches); 
$umsysdll_works = "{$matches[1]}";
preg_match("/Mature: (\d)/",$lines,$matches); 
$umsysdll_mature = "{$matches[1]}";

/* OEM Font */  													  
/* Should always be 100% dosen't need to grab data from status.txt file */ 

$oemfont_full_data = "100";
$oemfont_boots = "1";
$oemfont_works = "1";
$oemfont_mature = "1";

/* NLS Files */ 	  	  	  	  	  	  	  	  	  	  	  	  
/* Should always be 100% dosen't need to grab data from status.txt file */
$nlsfiles_full_data = "100";
$nlsfiles_boots = "1";
$nlsfiles_works = "1";
$nlsfiles_mature = "1";

?>
