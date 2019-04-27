
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
 
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<title>Operating Systems with  C/C++ 2012/13</title>
<script type="text/javascript" src="https://www.google.com/jsapi"></script>
<script type="text/javascript">
//<![CDATA[
google.load("jquery", "1.5.2");
//]]>
</script>

<link href="/SiteElements/CSS/0000reset.css" rel="stylesheet" type="text/css" />
<link href="/SiteElements/CSS/0100style.css" rel="stylesheet" type="text/css" />
<link href="/SiteElements/CSS/0200navi.css" rel="stylesheet" type="text/css" />
<link href="/SiteElements/CSS/0333conversion.css" rel="stylesheet" type="text/css" />
<link href="/SiteElements/CSS/0400nonstandard.css" rel="stylesheet" type="text/css" />
<link href="/SiteElements/CSS/0501searchresults.css" rel="stylesheet" type="text/css" />
<link href="/SiteElements/CSS/0605courses.css" rel="stylesheet" type="text/css" />
<link href="/SiteElements/CSS/0605structuredcontentlistingsearch.css" rel="stylesheet" type="text/css" />
<link href="/SiteElements/CSS/0615courselistingsearch.css" rel="stylesheet" type="text/css" />
<link href="/SiteElements/CSS/0800print.css" rel="stylesheet" type="text/css" media="print" />
		


<script type="text/javascript">
//<![CDATA[
if (typeof(window.$j) == 'undefined') { window.$j = $; }
window.$j.register = function(name) {if (!this._components){this._components = {};} this._components[name] = true;};
window.$j.isRegistered = function(name) { if (!this._components) { return false; } return !!(this._components[name]); };
window.$j.requires = function(name) { if (!this.isRegistered(name)) { alert('JQuery Extension " ' + name + '" not registered'); }};
if (typeof(jQuery.fn.setArray) == 'undefined') { jQuery.fn.setArray = function( elems ) { this.length = 0; jQuery.fn.push.apply(this, elems); return this; }};
//]]>
</script>
		
<script  type="text/javascript" src="/SiteElements/JavaScript/homepageFeatureControl.js?modified=2012-3-6-12-3&amp;build=711935"></script>
		
<script  type="text/javascript" src="/SiteElements/JavaScript/jquery-media.js?build=711935"></script>
		
<script  type="text/javascript" src="/SiteElements/JavaScript/navigation.js?build=711935"></script>
		
<script  type="text/javascript" src="/SiteElements/JavaScript/corefunctionsRevised.js?build=711935"></script>

<script type="text/javascript">
//<![CDATA[
var _gaq = _gaq || [];
_gaq.push(['_setAccount', "UA-18633482-1"]);
_gaq.push(['_trackPageview']);
_gaq.push(['_trackPageLoadTime']);
(function() {
var ga = document.createElement('script'); ga.type = 'text/javascript'; ga.async = true;
ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(ga, s);
})();
//]]>
</script>
 <link href="/sys/style-sheets/sys-styles.css" rel="stylesheet"
       type="text/css"/>

</head><body>

    
<div class="aspNetHidden">
<input type="hidden" name="ScriptManager_HiddenField" id="ScriptManager_HiddenField" value="" />
<input type="hidden" name="__VIEWSTATE" id="__VIEWSTATE" value="/wEPDwUJMjE2NjAzMTQ0D2QWAgIEEGRkFgQCAw8PFgQeCENzc0NsYXNzBRdzeXNfdGV4dEJveFdpdGhSZWRpcmVjdB4EXyFTQgICZBYEZg8PFgQeE0Fzc29jaWF0ZWRDb250cm9sSUQFBWN0bDIwHgRUZXh0BQZTZWFyY2hkZAICDw8WAh8DBQJHb2RkAgUPZBYCAgEPZBYCAgMQZGQWAmYQZGQWBGYQDxYEHwAFCXN5c19maXJzdB8BAgJkZGQCCBAPFgQfAAUIc3lzX2xhc3QfAQICZGRkZDqOVz62APtRG83Mt8vFdW+4UkLd" />
</div>

<script src="/sys/resource-cache/msajax.js" type="text/javascript"></script><noscript><p>Browser does not support script.</p></noscript>
        
    <div id="wrapper">
      <div id="header">
        <h1 id="logo">
          <a href="http://www.birmingham.ac.uk">University of Birmingham</a>
        </h1><div id="search">
     <div id="search">
      <form action="http://www.cs.bham.ac.uk/search/" method="get">
       <div class="sys_textBoxWithRedirect">
        <input type="hidden" name="site" value="www.cs.bham.ac.uk"/>
        <label for="ctl20">Search</label>
        <input id="ctl20" class="sys_searchBox" type="text" value="Search..." name="query">
        <input id="ctl21" class="sys_searchButton" type="submit" value="Go">
       </div>
      </form>
     </div>
</div>
</div>      <div id="main">
        <div id="navWrap">
<h1>School of Computer Science<br/>Personal Web Page</h1>
</div>        <div id="content">
                    <div id="breadcrumbs">
<a class="sys_0 sys_t3000" href="/">Computer Science</a>&nbsp;/&nbsp;<a href="/~exr/lectures" class="sys_0 sys_t166">Lectures</a>&nbsp;/&nbsp;<a href="/~exr/lectures/opsys" class="sys_0 sys_t166">Opsys</a>&nbsp;/&nbsp;<a href="/~exr/lectures/opsys/13_14" class="sys_0 sys_t166">13 14</a>&nbsp;/&nbsp;<span>Lectures</span>
</div>          <div id="mainNav">
<ul class="sys_simpleListMenu">
<li class="sys_first"><div class="sys_selected sys_currentitem"><a href="/">School of Computer Science<br/>Personal Web Page</a></div></li>
<li>
<a href="./">Module Home Page
</a>
</li>
<li>
<a href="lectures.php">Lecture notes plus examples used 
</a>
</li>
<li>
<a href="exercises.php">Exercises 
</a>
</li>
<li>
<a href="shell.php">Basic shell usage
</a>
</li>
<li>
<a href="documentation.php">Additional documentation
</a>
</li>
<li>
<a href="virtualMachine.php">Downloading and using a suitable Virtual Machine
</a>
</li>
<!--<li>
<a href="docs/kernelProgramming.php">How to compile and run you own kernel modules 
</a>
</li> -->
</ul>
</div><h1>

</h1><div id="wideContent">
<H1> Handouts and other Material for the lectures </H1>

<TABLE Border=2> 
<TR> <TD> Slides </TD>
     <TD> Examples  </TD>
</TR>
<TR> 
<!-- Line 1 -->
     <TD> <a href=lectures/os_01_intro.pdf> Introduction </a> </TD>
</TR>
<TR> 
<!-- Line 2 -->
     <TD> <a href=lectures/os_02_architecture.pdf> Operating Systems Architecture</a> </TD>
</TR>
<!-- Line 3 -->
     <TD> <a href=lectures/os_03_sys_prog_intro.pdf> First steps in C </a> </TD>
<TD><a href=examples/basics>Code </a></TD> </TR>
<!-- Line 4 -->
     <TD> <a href=lectures/os_04_pointers.pdf> Pointers </a> </TD>
<TD><a href=examples/pointers>Code </a></TD> </TR>
<!-- Line 5 -->
     <TD> <a href=lectures/os_05_files_and_structs.pdf> Files and Structs </a> </TD>
<TD><a href=examples/struct>Code </a></TD> </TR>
<!-- Line 6 -->
     <TD> <a href=lectures/os_06_processes.pdf> Processes </a> </TD>
<TD></TD> </TR>
<!-- Line 7 -->
     <TD> <a href=lectures/os_07_scheduling.pdf> Scheduling </a> </TD>
<TD></TD> </TR>
<!-- Line 8 -->
     <TD> <a href=lectures/os_08_threads.pdf> Threads </a> </TD>
<TD><a href=examples/processes>Code </a></TD> </TR>
<!-- Line 9 -->
     <TD>  <a href=lectures/os_09_sockets.pdf>Sockets</a> </TD>
<TD><a href=examples/sockets>Code </a></TD> </TR>
<!-- Line 10 -->
     <TD> <a href=lectures/os_10_build.pdf>Compiling large programs </a> </TD>
<TD><a href=examples/build>Code </a></TD> </TR>
<!-- Line 11 -->
     <TD> <a href=lectures/os_11_signals_etc.pdf>Signals </a> </TD>
<TD><a href=examples/signals>Code </a></TD> </TR>
<!-- Line 12 -->
<TR>     <TD> <a href=lectures/os_12_ipc.pdf>Inter-Process Communication </a> </TD></TR>
<!-- Line 13 -->
<TR>     <TD> <a href=lectures/os_13_sync.pdf>Process Synchronisation
</a> </TD> </TR>
<!-- Line 15 -->
<TR>     <TD> <a href=lectures/os_15_memory.pdf>Memory management </a> </TD> <TD></TD> </TR>
<!-- Line 16 -->
<TR>     <TD> <a href=lectures/os_16_kernelProgramming.pdf>Linux kernel programming </a> </TD> <TD><a href=examples/modules> Code </a></TD> </TR>
<!-- Line 17 -->
<TR>     <TD> <a href=lectures/os_17_c++.pdf>C++ </a> </TD> <TD><a href=examples/c++> Code </a></TD> </TR>
<!-- Line 18 -->
<TR>     <TD> <a href=lectures/os_18_filesys.pdf>File systems </a></TD> <TD></TD> </TR>
<!-- Line 19 -->
<TR>     <TD> <a href=lectures/os_19_deviceDriver.pdf>Device Drivers </a> </TD> <TD><a href=examples/deviceDriver> Code </a></TD> </TR>
<!-- Line 20 -->
<TR>     <TD> <a href=lectures/os_20_architecture.pdf>Operating Systems Architectures </a> </TD> <TD>></TD> </TR>
</TABLE>



</div>      </div>
    </div>
      <div id="footer"><ul id="footerBottom">
    <li class="sys_first">
        &#169; University of Birmingham 2012
    </li>
    <li>
        <a href="/privacy">Privacy</a>
    </li>
    <li>
        <a href="/legal">Legal</a>
    </li>
    <li><a href="/privacy/cookies.aspx">Cookies and cookie policy</a></li>
<li>
        <a href="/accessibility">Accessibility</a>
    </li>
 <li>
        <a href="/sitemap">Site map</a>
    </li>
<li>
        <a href="/contact/web-feedback.aspx">Website feedback</a>
    </li>
<li class="sys_last">
        <a href="/university/governance/publication-scheme/charitable.aspx">Charitable information</a>
    </li>
</ul>
        <p id="precedent" class="sys_fadeAway">
          <a href="http://supportweb.cs.bham.ac.uk">Delivered by Computer Science</a>
        </p>
      </div>
    </div>
    <div id="sliderWrapper">
      <!-- to be completed -->
    </div>
  
    
<div class="aspNetHidden">
	<input type="hidden" name="__EVENTVALIDATION" id="__EVENTVALIDATION" value="/wEWAwLNlYeGAQKhwOHWCQKiwOHWCQ+QGoDy3bDYj0qKk8z35xKBmk3R" />
</div>
<script type='text/javascript' src='/WebResource.axd?d=AJqRMpMBOvvUbTdyHRXnfS-z_Bl3b1TNDSov7qceMY2_vAOZWJus0L6_UgbRfmc8RhDkSmOgTG0jHuCsw49KqmSaoZ3f_fx5cL45yCBJYOD0UYOi6CpJkR2DEzN9Ey26z3SHitHpJC2Jqcr0T0AwApELwVM1&t=634702885533761893'></script><noscript><p>Browser does not support script.</p></noscript>
<script type='text/javascript'>jwplayer('FlashFLVPlayerMain1hh').setup({'flashplayer': '/WebResource.axd?d=Is6Cdw3SSzJGrF0JiD2KHWnDyOSI-UDusYRCteyyEuzYl5Yi_Z8x8sxJCV8kMMt6Vffk5w_TMpsGHyB_aWw3fskxmv2bjKW_2MVgehxGl3fHFNED2qnsRZfDwgOpxi2zaVbIAl464HLLKYx71aDzZc4Z4hU1&t=634702885533761893','modes':[{ 'type': 'flash','src': '/WebResource.axd?d=Is6Cdw3SSzJGrF0JiD2KHWnDyOSI-UDusYRCteyyEuzYl5Yi_Z8x8sxJCV8kMMt6Vffk5w_TMpsGHyB_aWw3fskxmv2bjKW_2MVgehxGl3fHFNED2qnsRZfDwgOpxi2zaVbIAl464HLLKYx71aDzZc4Z4hU1&t=634702885533761893','config': {'file': 'http://www.youtube.com/watch?v=8UlW_7dg4KI'} } ,{ 'type': 'html5'} ,{ 'type': 'download'}] ,'controlbar': 'Bottom','width': '235px','height': '241px','autostart': 'False','mute': 'False','volume': '90','icons': 'True','stretching': 'Uniform'});</script><noscript><p>Browser does not support script.</p></noscript>
<script type="text/javascript">
//<![CDATA[
		(function ($) {
    var redirectBox = $('#ctl20');
    var goButton = $('#ctl21')
    var defaultSearchTerms = redirectBox[0].value;
    var currentSearchTerms = defaultSearchTerms;
    // Ensures that the correct event is raised on the server if the <ENTER> key is pressed
    $(redirectBox).keypress(function (e) {
        if (e.keyCode == 13) {
            e.preventDefault();
            $(goButton).click();
        }
    })
    // If the search box gets focus and the value is the default keywords 
    // initially set when the page was requested the value is set to blank
    redirectBox.focus(function () {
        if (redirectBox[0].value === defaultSearchTerms) {
            redirectBox.toggleClass('init');
            redirectBox[0].value = "";
        }
    });
    // If the search box loses focus and the value is blank then this will
    // repopulate the search box value with the default keywords initially set
    // when the page was requested
    redirectBox.blur(function () {
        if (redirectBox[0].value === "") {
            if (currentSearchTerms == defaultSearchTerms) {
                redirectBox.toggleClass('init');
            }
            redirectBox[0].value = currentSearchTerms;
        }
    });
})(jQuery);
//]]>
</script><noscript><p>Browser does not support script.</p></noscript></body>
</html>yes
