<?php

/* TinyKrnl Project Status - Phase 1 Overview - Status Script
 * File - status.php 
 * Written by Peter Johansson. */

/* This file is the output of the script (The page the user will see on he's / her webbrowser)
 * In every module table PHP will output the orange boxes HTML from the module_status variable.
 */

/* Change the location to the CSS files below
 * I entered the full HTTP path to those CSS files inorder to test the design on my Webserver
 *
 * Regards
 * Peter Johansson
 */

include 'files.php';
include 'definitions.php';

?>

<title>TinyKRNL Project Status - Phase 1 Overview</title>

<link rel="stylesheet" type="text/css" href="http://www.tinykrnl.org/sinorca-screen.css" media="screen" title="Sinorca (screen)" />
<link rel="stylesheet alternative" type="text/css" href="http://www.tinykrnl.org/sinorca-screen-alt.css" media="screen" title="Sinorca(alternative)" />
<link rel="stylesheet" type="text/css" href="http://www.tinykrnl.org/sinorca-print.css" media="print" />

<div id="main-copy">

<h1 id="introduction">Phase 1 Status</h1>
<div align="left">

<table border="0" cellpadding="0" cellspacing="1">
<tr valign="top">
<td nowrap>&nbsp;</td>

<td nowrap width="50" align="center">
<font class="statusheader">Assigned</font></td>

<td nowrap width="50" align="center" colspan="10">
<font class=statusheader>Development</td>

<td nowrap width="50" align="center">
<font class="statusheader">Boots</font></td>

<td nowrap width="50" align="center">
<font class="statusheader">Works</font></td>

<td nowrap width="50" align="center">
<font class="statusheader">Mature</font></td>
</tr>

<tr>
<td align="right" nowrap="nowrap">
&nbsp;</td> <td bgcolor="#ff6700">
</td>

<td bgcolor="#f59427" colspan="10">
&nbsp;</td> <td bgcolor="#ffb81e">
</td>

<th colspan="1" bgcolor="#eee42f" nowrap="nowrap"></th>
<th colspan="1" bgcolor="#2e9028" nowrap="nowrap">&nbsp;</th>
</tr>

<tr>
<td align="right" nowrap="nowrap">
<a id="statusteam" href="atapi.php">
ATAPI</a>&nbsp; </td>
<?php echo $atapi_status; ?>
<?php if ($atapi_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($atapi_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($atapi_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="bootvid.php">
BootVid</a>&nbsp; </td>
<?php echo $bootvid_status; ?>
<?php if ($bootvid_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($bootvid_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($bootvid_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="classpnp.php">
ClassPnP</a>&nbsp; </td>
<?php echo $classpnp_status; ?>
<?php if ($classpnp_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($classpnp_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($classpnp_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="disk.php">
Disk</a>&nbsp; </td>
<?php echo $disk_status; ?>
<?php if ($disk_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($disk_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($disk_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="fastfat.php">
FastFAT</a>&nbsp; </td>
<?php echo $fastfat_status; ?>
<?php if ($fastfat_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($fastfat_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($fastfat_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>
					
<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="ftdisk.php">
FtDisk</a>&nbsp; </td>
<?php echo $ftdisk_status; ?>
<?php if ($ftdisk_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($ftdisk_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($ftdisk_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="i8042prt.php">
i8042prt</a>&nbsp; </td>
<?php echo $i8042prt_status; ?>
<?php if ($i8042prt_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($i8042prt_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($i8042prt_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="isapnp.php">
ISAPnP</a>&nbsp; </td>
<?php echo $isapnp_status; ?>
<?php if ($isapnp_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($isapnp_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($isapnp_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

					
<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="isapnp.php">
IntelIDE</a>&nbsp; </td>
<?php echo $intelide_status; ?>
<?php if ($intelide_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($intelide_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($intelide_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="kbdclass.php">
KbdClass</a>&nbsp; </td>
<?php echo $kdbclass_status; ?>
<?php if ($kdbclass_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($kdbclass_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($kdbclass_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>
					
<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="kdcom.php">
KDCom</a>&nbsp; </td>
<?php echo $kdcom_status; ?>
<?php if ($kdcom_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($kdcom_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($kdcom_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>
					
<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="mountmgr.php">
MountMgr</a>&nbsp; </td>
<?php echo $mountmgr_status; ?>
<?php if ($mountmgr_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($mountmgr_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($mountmgr_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="partmgr.php">
PartMgr</a>&nbsp; </td>
<?php echo $partmgr_status; ?>
<?php if ($partmgr_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($partmgr_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($partmgr_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>
					
<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="pci.php">
PCI</a>&nbsp; </td>
<?php echo $pci_status; ?>
<?php if ($pci_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($pci_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($pci_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="pciidex.php">
PCIIDEx</a>&nbsp; </td>
<?php echo $pciidex_status; ?>
<?php if ($pciidex_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($pciidex_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($pciidex_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="wmilib.php">
WMILib</a>&nbsp; </td>
<?php echo $wmilib_status; ?>
<?php if ($wmilib_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($wmilib_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($wmilib_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam" href="hal.php">HAL</a>&nbsp; </td>
<?php echo $hal_status; ?>
<?php if ($hal_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($hal_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($hal_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam0" href="kernel.php">Kernel</a>&nbsp; </td>
<?php echo $kernel_status; ?>
<?php if ($kernel_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($kernel_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($kernel_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam1" href="ntdll.php">UM Sys. 
DLL</a>&nbsp; </td>
<?php echo $umsysdll_status; ?>
<?php if ($umsysdll_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($umsysdll_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($umsysdll_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam">
OEM Font</a>&nbsp; </td>
<?php echo $oemfont_status; ?>
<?php if ($oemfont_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($oemfont_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($oemfont_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

<tr>
<td align="right" nowrap="nowrap" width="79">
<a id="statusteam">
NLS Files</a>&nbsp; </td>
<?php echo $nlsfiles_status; ?>
<?php if ($nlsfiles_boots == 1)
{
echo "<td bgcolor=#ffb81e>
</td>";
}
if ($nlsfiles_works == 1)
{
echo "<th colspan=1 bgcolor=#eee42f nowrap=nowrap></th>";
}
if ($nlsfiles_mature == 1)
{
echo "<th colspan=1 bgcolor=#2e9028 nowrap=nowrap>&nbsp;</th>";
}
?>
</tr>

</table>
</div><p>
