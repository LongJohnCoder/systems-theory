<?php

/* TinyKrnl Project Status - Modules Overview Script
 * File - atapi.php
 * Written by Peter Johansson. */


/* Module Description - Line 35 */
$description = "ATAPI Description Goes Here...";

/* Files Definition Location */
$file = "http://svn.tinykrnl.org/svn/tinykrnl/drivers/storage/ide/atapi/atstatus.txt?view=co";
$lines = file_get_contents($file);

/* EDIT ONLY THE FILE LOCATION
 * Due that the Function Implementation Status and Module Implementation Status
 * Uses two different of actions when opening the file, so dont put the line in a oneliner
 * Only edit the file location.
 *
 *  Regards
 *  Peter Johansson
 */


/* Module Implementation Status - ( Bottom Of This Page - Line 115 ) */
	preg_match("/TOTAL      : (\d\d\d)/",$lines,$matches); 
$total = "{$matches[0]}";
	preg_match("/IMPLEMENTED: (\d\d\d)/",$lines,$matches); 
$implemented = "{$matches[0]}";
	preg_match("/% COMPLETED: (\d\d\d)/",$lines,$matches); 
$completed = "{$matches[0]}";

/* Function Implementation Status ( Look below for more information - Line 64 ) */
$function_list = fopen($file, "r");
?>

<link rel="stylesheet" type="text/css" href="http://www.tinykrnl.org/sinorca-screen.css" media="screen" title="Sinorca (screen)" />
<link rel="stylesheet alternative" type="text/css" href="http://www.tinykrnl.org/sinorca-screen-alt.css" media="screen" title="Sinorca (alternative)" />
<link rel="stylesheet" type="text/css" href="http://www.tinykrnl.org/sinorca-print.css" media="print" />

<div id="main-copy">
<h1 id="welcome">Module Description</h1>
<p> <?php echo $description; ?> </p>

<h1 id="obtain">Function Implementation Status</h1>
<p>The following table lists the implementation status of each function in this module.</p>

<table border="0" cellpadding="0" cellspacing="0" width="628" style="border-collapse: collapse;width:472pt">

<colgroup>
<col width="301" style="width: 226pt">
<col width="50" style="width: 38pt">
<col width="72" span="2" style="width:54pt">
<col width="133" style="width: 100pt">
</colgroup>

<tr height="20" style="height:15.0pt">
<td height="20" width="301" style="height: 15.0pt; font-size: 11.0pt; color: white; font-weight: 700; text-decoration: none; text-underline-style: none; font-style: normal; font-family: Calibri; text-align: general; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: #C0504D">
Function Name</td>

<td width="50" style="width: 38pt; font-size: 11.0pt; color: white; font-weight: 700; text-decoration: none; text-underline-style: none; text-align: center; font-style: normal; font-family: Calibri; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: #C0504D">
Section</td>

<td width="72" style="width: 54pt; font-size: 11.0pt; color: white; font-weight: 700; text-decoration: none; text-underline-style: none; text-align: center; font-style: normal; font-family: Calibri; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: #C0504D">
Start</td>

<td width="72" style="width: 54pt; font-size: 11.0pt; color: white; font-weight: 700; text-decoration: none; text-underline-style: none; text-align: center; font-style: normal; font-family: Calibri; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: #C0504D">
End</td>

<td width="133" style="width: 100pt; font-size: 11.0pt; color: white; font-weight: 700; text-decoration: none; text-underline-style: none; text-align: center; font-style: normal; font-family: Calibri; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: #C0504D">
Status</td>
</tr>

<tr height="20" style="height:15.0pt">
<?php 

while ($content = fscanf($function_list, "%s\t%s\t%s\t%s\t%[^[]\t[%[^]]] \n")) {
   list ($function_name, $section, $start, $end, $misc, $status) = $content;

$color_check++;

if($color_check % 2 == 1) 
{ 
$color = "#ADC0D9";
}

if($color_check % 2 == 0)
{ 
$color = "#D6DFEC";
}
?>

<td height="20" style="height: 15.0pt; tcolor: black; font-size: 11.0pt; font-weight: 400; font-style: normal; text-decoration: none; font-family: Calibri; text-align: general; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: <?php echo $color; ?>">

<?php echo $function_name; ?>
</td>

<td height="20" style="height: 15.0pt; color: black; font-size: 11.0pt; font-weight: 400; font-style: normal; text-decoration: none; font-family: Calibri; text-align: general; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: <?php echo $color; ?>">

<?php echo $section; ?>
</td>

<td height="20" style="height: 15.0pt; color: black; font-size: 11.0pt; font-weight: 400; font-style: normal; text-decoration: none; font-family: Calibri; text-align: general; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: <?php echo $color; ?>">

<?php echo $start; ?>
</td>

<td height="20" style="height: 15.0pt; color: black; font-size: 11.0pt; font-weight: 400; font-style: normal; text-decoration: none; font-family: Calibri; text-align: general; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: <?php echo $color; ?>">

<?php echo $end; ?>
</td>

<td height="20" style="height: 15.0pt; text-align: center; color: black; font-size: 11.0pt; font-weight: 400; font-style: normal; text-decoration: none; font-family: Calibri; text-align: general; vertical-align: bottom; white-space: nowrap; border: medium none; padding-left: 1px; padding-right: 1px; padding-top: 1px; background: <?php echo $color; ?>">

<?php echo $status; ?>
</td>
</tr>

<?php
}
fclose($function_list);

?>
</table>

<h1 id="obtain0">Module Implementation Status</h1>
<table style="border-collapse: collapse; width: 226pt;" border="0" cellpadding="0" cellspacing="0" width="301">

<tr style="height: 15pt;" height="20">
<td><?php echo $total; ?></td>

<tr style="height: 15pt;" height="20">
<td><?php echo $implemented; ?></td>

<tr style="height: 15pt;" height="20">
<td><?php echo $completed; ?></td>

</tr>
</table>
</div>
</div>
