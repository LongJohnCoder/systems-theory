<?php

/* TinyKrnl Project Status - Phase 1 Overview - Status Script
 * File - definitions.php
 * Written by Peter Johansson. */

/* This file checks the finished procentage of each module and 
 * adds the orange boxes HTML code into a variable.
 */

/* Yes i know this file is hudge ~ 3000 lines of code.
 * You maybe wonder how long time it will take to edit this file when the phase 2 development start.
 * Well it really dosen't take that long time every texteditor have a replace function.
 * So when the phase 2 development starts, just bring up the replace function in your texteditor
 * Example replace every 
 * $atapi to $phase2-module-name 
 * after that
 * replace every $bootvid to $phase2-module2-name
 * It wont take more than 5 minutes, so i really dont think it should be any problem at all ;)
 * And yes ~ 3000 lines of code dosen't eat CPU usage of your webserver. = D
 *
 * Regards
 * Peter Johansson
 */

/* ATAPI */
if ($atapi >= 10 && $atapi < 20 && $atapi_full_data != 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($atapi >= 20 && $atapi < 30 && $atapi_full_data != 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($atapi >= 30 && $atapi < 40 && $atapi_full_data != 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($atapi >= 40 && $atapi < 50 && $atapi_full_data != 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($atapi >= 50 && $atapi < 60 && $atapi_full_data != 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($atapi >= 60 && $atapi < 70 && $atapi_full_data != 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($atapi >= 70 && $atapi < 80 && $atapi_full_data != 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($atapi >= 80  && $atapi < 90 && $atapi_full_data != 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($atapi >= 90  && $atapi < 100 && $atapi_full_data != 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($atapi_full_data == 100)
{
$atapi_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* BootVid */  	  													 
if ($bootvid >= 10 && $bootvid < 20 && $bootvid_full_data != 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($bootvid >= 20 && $bootvid < 30 && $bootvid_full_data != 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($bootvid >= 30 && $bootvid < 40 && $bootvid_full_data != 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($bootvid >= 40 && $bootvid < 50 && $bootvid_full_data != 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($bootvid >= 50 && $bootvid < 60 && $bootvid_full_data != 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($bootvid >= 60 && $bootvid < 70 && $bootvid_full_data != 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($bootvid >= 70 && $bootvid < 80 && $bootvid_full_data != 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($bootvid >= 80  && $bootvid < 90 && $bootvid_full_data != 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($bootvid >= 90  && $bootvid < 100 && $bootvid_full_data != 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($bootvid_full_data == 100)
{
$bootvid_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* ClassPnP */  	  											  	 
if ($classpnp >= 10 && $classpnp < 20 && $classpnp_full_data != 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($classpnp >= 20 && $classpnp < 30 && $classpnp_full_data != 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($classpnp >= 30 && $classpnp < 40 && $classpnp_full_data != 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($classpnp >= 40 && $classpnp < 50 && $classpnp_full_data != 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($classpnp >= 50 && $classpnp < 60 && $classpnp_full_data != 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($classpnp >= 60 && $classpnp < 70 && $classpnp_full_data != 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($classpnp >= 70 && $classpnp < 80 && $classpnp_full_data != 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($classpnp >= 80  && $classpnp < 90 && $classpnp_full_data != 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($classpnp >= 90  && $classpnp < 100 && $classpnp_full_data != 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($classpnp_full_data == 100)
{
$classpnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* Disk */  	  											  	  	 
if ($disk >= 10 && $disk < 20 && $disk_full_data != 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($disk >= 20 && $disk < 30 && $disk_full_data != 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($disk >= 30 && $disk < 40 && $disk_full_data != 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($disk >= 40 && $disk < 50 && $disk_full_data != 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($disk >= 50 && $disk < 60 && $disk_full_data != 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($disk >= 60 && $disk < 70 && $disk_full_data != 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($disk >= 70 && $disk < 80 && $disk_full_data != 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($disk >= 80  && $disk < 90 && $disk_full_data != 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($disk >= 90  && $disk < 100 && $disk_full_data != 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($disk_full_data == 100)
{
$disk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* FastFAT */  	  											  	  	 
if ($fastfat >= 10 && $fastfat < 20 && $fastfat_full_data != 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($fastfat >= 20 && $fastfat < 30 && $fastfat_full_data != 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($fastfat >= 30 && $fastfat < 40 && $fastfat_full_data != 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($fastfat >= 40 && $fastfat < 50 && $fastfat_full_data != 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($fastfat >= 50 && $fastfat < 60 && $fastfat_full_data != 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($fastfat >= 60 && $fastfat < 70 && $fastfat_full_data != 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($fastfat >= 70 && $fastfat < 80 && $fastfat_full_data != 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($fastfat >= 80  && $fastfat < 90 && $fastfat_full_data != 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($fastfat >= 90  && $fastfat < 100 && $fastfat_full_data != 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($fastfat_full_data == 100)
{
$fastfat_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* FtDisk */  	  											  	  	 
if ($ftdisk >= 10 && $ftdisk < 20 && $ftdisk_full_data != 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($ftdisk >= 20 && $ftdisk < 30 && $ftdisk_full_data != 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($ftdisk >= 30 && $ftdisk < 40 && $ftdisk_full_data != 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($ftdisk >= 40 && $ftdisk < 50 && $ftdisk_full_data != 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($ftdisk >= 50 && $ftdisk < 60 && $ftdisk_full_data != 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($ftdisk >= 60 && $ftdisk < 70 && $ftdisk_full_data != 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($ftdisk >= 70 && $ftdisk < 80 && $ftdisk_full_data != 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($ftdisk >= 80  && $ftdisk < 90 && $ftdisk_full_data != 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($ftdisk >= 90  && $ftdisk < 100 && $ftdisk_full_data != 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($ftdisk_full_data == 100)
{
$ftdisk_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* i8042prt */  	  											  	  
if ($i8042prt >= 10 && $i8042prt < 20 && $i8042prt_full_data != 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($i8042prt >= 20 && $i8042prt < 30 && $i8042prt_full_data != 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($i8042prt >= 30 && $i8042prt < 40 && $i8042prt_full_data != 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($i8042prt >= 40 && $i8042prt < 50 && $i8042prt_full_data != 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($i8042prt >= 50 && $i8042prt < 60 && $i8042prt_full_data != 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($i8042prt >= 60 && $i8042prt < 70 && $i8042prt_full_data != 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($i8042prt >= 70 && $i8042prt < 80 && $i8042prt_full_data != 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($i8042prt >= 80  && $i8042prt < 90 && $i8042prt_full_data != 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($i8042prt >= 90  && $i8042prt < 100 && $i8042prt_full_data != 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($i8042prt_full_data == 100)
{
$i8042prt_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* ISAPnP */   			  	  	  	  	  	  	  			 
if ($isapnp >= 10 && $isapnp < 20 && $isapnp_full_data != 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($isapnp >= 20 && $isapnp < 30 && $isapnp_full_data != 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($isapnp >= 30 && $isapnp < 40 && $isapnp_full_data != 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($isapnp >= 40 && $isapnp < 50 && $isapnp_full_data != 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($isapnp >= 50 && $isapnp < 60 && $isapnp_full_data != 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($isapnp >= 60 && $isapnp < 70 && $isapnp_full_data != 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($isapnp >= 70 && $isapnp < 80 && $isapnp_full_data != 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($isapnp >= 80  && $isapnp < 90 && $isapnp_full_data != 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($isapnp >= 90  && $isapnp < 100 && $isapnp_full_data != 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($isapnp_full_data == 100)
{
$isapnp_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* IntelIDE */										 
if ($intelide >= 10 && $intelide < 20 && $intelide_full_data != 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($intelide >= 20 && $intelide < 30 && $intelide_full_data != 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($intelide >= 30 && $intelide < 40 && $intelide_full_data != 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($intelide >= 40 && $intelide < 50 && $intelide_full_data != 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($intelide >= 50 && $intelide < 60 && $intelide_full_data != 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($intelide >= 60 && $intelide < 70 && $intelide_full_data != 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($intelide >= 70 && $intelide < 80 && $intelide_full_data != 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($intelide >= 80  && $intelide < 90 && $intelide_full_data != 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($intelide >= 90  && $intelide < 100 && $intelide_full_data != 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($intelide_full_data == 100)
{
$intelide_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* KbdClass */ 	  										  	  	  	 
if ($kbdclass >= 10 && $kbdclass < 20 && $kbdclass_full_data != 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kbdclass >= 20 && $kbdclass < 30 && $kbdclass_full_data != 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($kbdclass >= 30 && $kbdclass < 40 && $kbdclass_full_data != 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kbdclass >= 40 && $kbdclass < 50 && $kbdclass_full_data != 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kbdclass >= 50 && $kbdclass < 60 && $kbdclass_full_data != 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kbdclass >= 60 && $kbdclass < 70 && $kbdclass_full_data != 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kbdclass >= 70 && $kbdclass < 80 && $kbdclass_full_data != 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kbdclass >= 80  && $kbdclass < 90 && $kbdclass_full_data != 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kbdclass >= 90  && $kbdclass < 100 && $kbdclass_full_data != 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($kbdclass_full_data == 100)
{
$kbdclass_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* KDCom */  													 
if ($kdcom >= 10 && $kdcom < 20 && $kdcom_full_data != 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kdcom >= 20 && $kdcom < 30 && $kdcom_full_data != 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($kdcom >= 30 && $kdcom < 40 && $kdcom_full_data != 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kdcom >= 40 && $kdcom < 50 && $kdcom_full_data != 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kdcom >= 50 && $kdcom < 60 && $kdcom_full_data != 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kdcom >= 60 && $kdcom < 70 && $kdcom_full_data != 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kdcom >= 70 && $kdcom < 80 && $kdcom_full_data != 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kdcom >= 80  && $kdcom < 90 && $kdcom_full_data != 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kdcom >= 90  && $kdcom < 100 && $kdcom_full_data != 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($kdcom_full_data == 100)
{
$kdcom_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* MountMgr */  													 
if ($mountmgr >= 10 && $mountmgr < 20 && $mountmgr_full_data != 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($mountmgr >= 20 && $mountmgr < 30 && $mountmgr_full_data != 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($mountmgr >= 30 && $mountmgr < 40 && $mountmgr_full_data != 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($mountmgr >= 40 && $mountmgr < 50 && $mountmgr_full_data != 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($mountmgr >= 50 && $mountmgr < 60 && $mountmgr_full_data != 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($mountmgr >= 60 && $mountmgr < 70 && $mountmgr_full_data != 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($mountmgr >= 70 && $mountmgr < 80 && $mountmgr_full_data != 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($mountmgr >= 80  && $mountmgr < 90 && $mountmgr_full_data != 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($mountmgr >= 90  && $mountmgr < 100 && $mountmgr_full_data != 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($mountmgr_full_data == 100)
{
$mountmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* PartMgr */  													
if ($partmgr >= 10 && $partmgr < 20 && $partmgr_full_data != 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($partmgr >= 20 && $partmgr < 30 && $partmgr_full_data != 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($partmgr >= 30 && $partmgr < 40 && $partmgr_full_data != 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($partmgr >= 40 && $partmgr < 50 && $partmgr_full_data != 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($partmgr >= 50 && $partmgr < 60 && $partmgr_full_data != 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($partmgr >= 60 && $partmgr < 70 && $partmgr_full_data != 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($partmgr >= 70 && $partmgr < 80 && $partmgr_full_data != 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($partmgr >= 80  && $partmgr < 90 && $partmgr_full_data != 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($partmgr >= 90  && $partmgr < 100 && $partmgr_full_data != 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($partmgr_full_data == 100)
{
$partmgr_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* PCI */  											  	  	 
if ($pci >= 10 && $pci < 20 && $pci_full_data != 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pci >= 20 && $pci < 30 && $pci_full_data != 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($pci >= 30 && $pci < 40 && $pci_full_data != 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pci >= 40 && $pci < 50 && $pci_full_data != 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pci >= 50 && $pci < 60 && $pci_full_data != 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pci >= 60 && $pci < 70 && $pci_full_data != 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pci >= 70 && $pci < 80 && $pci_full_data != 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pci >= 80  && $pci < 90 && $pci_full_data != 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pci >= 90  && $pci < 100 && $pci_full_data != 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($pci_full_data == 100)
{
$pci_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* PCIIDEx */  											  	  	 
if ($pciidex >= 10 && $pciidex < 20 && $pciidex_full_data != 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pciidex >= 20 && $pciidex < 30 && $pciidex_full_data != 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($pciidex >= 30 && $pciidex < 40 && $pciidex_full_data != 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pciidex >= 40 && $pciidex < 50 && $pciidex_full_data != 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pciidex >= 50 && $pciidex < 60 && $pciidex_full_data != 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pciidex >= 60 && $pciidex < 70 && $pciidex_full_data != 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pciidex >= 70 && $pciidex < 80 && $pciidex_full_data != 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pciidex >= 80  && $pciidex < 90 && $pciidex_full_data != 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($pciidex >= 90  && $pciidex < 100 && $pciidex_full_data != 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($pciidex_full_data == 100)
{
$pciidex_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* WMILib */  													
if ($wmilib >= 10 && $wmilib < 20 && $wmilib_full_data != 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($wmilib >= 20 && $wmilib < 30 && $wmilib_full_data != 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($wmilib >= 30 && $wmilib < 40 && $wmilib_full_data != 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($wmilib >= 40 && $wmilib < 50 && $wmilib_full_data != 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($wmilib >= 50 && $wmilib < 60 && $wmilib_full_data != 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($wmilib >= 60 && $wmilib < 70 && $wmilib_full_data != 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($wmilib >= 70 && $wmilib < 80 && $wmilib_full_data != 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($wmilib >= 80  && $wmilib < 90 && $wmilib_full_data != 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($wmilib >= 90  && $wmilib < 100 && $wmilib_full_data != 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($wmilib_full_data == 100)
{
$wmilib_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* HAL */  			  	  	  	  	  	  	  	  	  	  	 
if ($hal >= 10 && $hal < 20 && $hal_full_data != 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($hal >= 20 && $hal < 30 && $hal_full_data != 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($hal >= 30 && $hal < 40 && $hal_full_data != 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($hal >= 40 && $hal < 50 && $hal_full_data != 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($hal >= 50 && $hal < 60 && $hal_full_data != 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($hal >= 60 && $hal < 70 && $hal_full_data != 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($hal >= 70 && $hal < 80 && $hal_full_data != 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($hal >= 80  && $hal < 90 && $hal_full_data != 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($hal >= 90  && $hal < 100 && $hal_full_data != 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($hal_full_data == 100)
{
$hal_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* Kernel */  			  	  	  	  	  	  	  	  	  	  	 
if ($kernel >= 10 && $kernel < 20 && $kernel_full_data != 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kernel >= 20 && $kernel < 30 && $kernel_full_data != 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($kernel >= 30 && $kernel < 40 && $kernel_full_data != 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kernel >= 40 && $kernel < 50 && $kernel_full_data != 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kernel >= 50 && $kernel < 60 && $kernel_full_data != 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kernel >= 60 && $kernel < 70 && $kernel_full_data != 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kernel >= 70 && $kernel < 80 && $kernel_full_data != 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kernel >= 80  && $kernel < 90 && $kernel_full_data != 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($kernel >= 90  && $kernel < 100 && $kernel_full_data != 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($kernel_full_data == 100)
{
$kernel_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* UM Sys.DLL */						  	  	  	  	  	  	  	  
if ($umsysdll >= 10 && $umsysdll < 20 && $umsysdll_full_data != 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($umsysdll >= 20 && $umsysdll < 30 && $umsysdll_full_data != 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";

}
if ($umsysdll >= 30 && $umsysdll < 40 && $umsysdll_full_data != 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($umsysdll >= 40 && $umsysdll < 50 && $umsysdll_full_data != 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($umsysdll >= 50 && $umsysdll < 60 && $umsysdll_full_data != 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($umsysdll >= 60 && $umsysdll < 70 && $umsysdll_full_data != 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($umsysdll >= 70 && $umsysdll < 80 && $umsysdll_full_data != 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($umsysdll >= 80  && $umsysdll < 90 && $umsysdll_full_data != 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}
if ($umsysdll >= 90  && $umsysdll < 100 && $umsysdll_full_data != 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

elseif ($umsysdll_full_data == 100)
{
$umsysdll_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* OEM Font */  													 
if ($oemfont_full_data == 100)
{
$oemfont_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

/* NLS Files */ 	  	  	  	  	  	  	  	  	  	  	  	 
if ($nlsfiles_full_data == 100)
{
$nlsfiles_status = "<td bgcolor=#ff6700>
&nbsp;</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>
<td bgcolor=#f59427 width=12>
</td>";
}

?>
