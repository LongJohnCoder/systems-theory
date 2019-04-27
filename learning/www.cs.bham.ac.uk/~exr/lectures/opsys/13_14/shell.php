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
<a class="sys_0 sys_t3000" href="/">Computer Science</a>&nbsp;/&nbsp;<a href="/~exr/lectures" class="sys_0 sys_t166">Lectures</a>&nbsp;/&nbsp;<a href="/~exr/lectures/opsys" class="sys_0 sys_t166">Opsys</a>&nbsp;/&nbsp;<a href="/~exr/lectures/opsys/13_14" class="sys_0 sys_t166">13 14</a>&nbsp;/&nbsp;<span>Shell</span>
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
<H1> Basic Shell Commands </H1>
<P>

The Unix shell is a powerful command interpreter. It is an essential
tool for systems programming. <BR>

The shell can be started from the menu via <TT>Xtras-&gt;Accesssories-&gt;Terminals.</TT>. 
You may now type commands into the shell which are then
executed. These commands may be either programs or commands which are
executed directly by the shell. <P>

The shell maintains a so-called <I> current directory </I>, which is
pre-prended to each file name given on the command line. When a new
shell is started, the current directory is the home directory. This
directory may be abbreviated with <TT>~</TT>. 

The most important commands are:
<UL>
<LI> <TT> cd &lt;directory&gt;</TT>: This command changes the current
  directory to the directory <TT> &lt;directory&gt;</TT>;
<LI> <TT> ls &lt;directory&gt;</TT>: This command lists all the files in the
  directory  <TT>&lt;directory&gt;</TT>. If this argument is omitted, the files
  in the current directory are listed;
<LI><TT>rm &lt;file&gt;</TT>: removes file <TT>&lt;file&gt;</TT>;
<LI&gt;<TT>cp &lt;file1&gt; &lt;file2&gt;</TT>: copies <TT>&lt;file1&gt;</TT> to <TT> &lt;file2&gt;</TT>. 
</UL>
<P>
Normally, the shell is not ready to execute the next command until the previous one has finished. If you only want to start the command and be able to issue further commands immediately, then you need to add  &amp; at the end of the command.
<P>
It is also possible to store the output of a command in a file for later analysis. This is achieved by adding &gt; &lt;file&gt; at the end of the command. This process is called <I> redirecting the output </I>. <P>

If you want to start programs in the current directory, you must precede the program name with ./ <P>


You do not have to type long filenames all yourself. It suffices to
type only part of the name 
and then press the key labelled &quot;Tab&quot;. The shell then  automatically
adds the rest of the name as far as it is unique. For example, assume
the current directory contains two files, test1a and test2b. If you
type &quot;t&quot; and then &quot;Tab&quot;, the shell will add the
string &quot;est&quot;. If you type now &quot;1&quot; and then the
Tab-key, emacs will add the character &quot;a&quot; and a space to
indicate that the filename is now complete. This feature is called <I>
file completion </I>, and is extremely useful.


For further information see the section entitled &quot; Shell &amp; Scripting &quot; of the <a href="http://supportweb.cs.bham.ac.uk/linux"> Using Linux </a> page of the supportweb.


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
</html>