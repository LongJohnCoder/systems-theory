<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
 
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<title>Operating Systems with  C/C++ 2010/11</title>
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
<a class="sys_0 sys_t3000" href="/">Computer Science</a>&nbsp;/&nbsp;<a href="/~exr/lectures" class="sys_0 sys_t166">Lectures</a>&nbsp;/&nbsp;<a href="/~exr/lectures/opsys" class="sys_0 sys_t166">Opsys</a>&nbsp;/&nbsp;<a href="/~exr/lectures/opsys/10_11" class="sys_0 sys_t166">10 11</a>&nbsp;/&nbsp;<span>Boss</span>
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
<a href="exercises.php">Exercises (assessed and non-assessed) 
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
<a href="boss.php">Submission System
</a>
</li>
<!-- <li>
<a href="docs/robotLab.php">How to use the PCs in the Robot Lab 
</a>
</li>
<li>
<a href="docs/kernelProgramming.php">How to compile and run you own kernel modules 
</a>
</li> -->
</ul>
</div><h1>

</h1><div id="wideContent">
<H1>BOSS</h1>

You will submit your exercises online using the School's coursework
submission system, BOSS. 
A quick guide to getting started
with BOSS is given below. More details are available from the
<a href="http://boss.cs.bham.ac.uk/">BOSS website</a>.

<h2>Getting started</h2>

BOSS online can be found at 
<a href="https://boss.cs.bham.ac.uk/BOSSonline/student/login.jsp" target="_blank">
https://boss.cs.bham.ac.uk/BOSSonline/student/login.jsp</a>. If you 
have not used BOSS before, you will need to request a password.

<h2>Requesting a password</h2>

<p>
From the BOSS online login screen, click on "New User or Forgot Password" 
(to the left of the screen). Enter your 6-digit student ID number
and surname where indicated and click Submit. A new password will
be emailed to you.
</p>
<p>
(Your student ID number appears on your ID card, where it is printed
with a leading zero so appears to have 7 digits).
</p>

<h2>Logging in</h2>

<p>
On the BOSS online login screen, enter your 6-digit student ID number
and password where indicated, and click Login. If you have just requested
a new password, at this point you should change it to something that you 
can remember - but it is a good idea not to re-use your network (Linux) 
password.
</p>
<p>
To change your password, click on "Change password" (to the left of the 
screen). Choose a new password and enter it in the "New password" and 
"Retype password" fields. Click "Change Password" to complete the process.
</p>

<h2>Submitting an assignment</h2>

To submit an assignment, from the BOSS main screen click "Submit or Test
an Assignment". This is a four-step process:

<ol>
<li><b>Choose problem</b> The choose problem screen contains three panels
entitled <i>Module</i>, <i>Assessment</i> and <i>Problem</i>. In the 
<i>Module</i> panel, click on <b>23636: Operating Systems with C/C++</b>.
Exercises for the module will appear in the <i>Assessment</i> panel; click
on the relevant exercise. Finally, click on the single entry now displayed
in the <i>Problem</i> panel. The "Confirm" button will now be enabled;
click it.
</li>
<li><b>Choose Files</b> This screen allows you to select the files 
that you wish to submit. Click the "Browse" button to look for a file to 
submit. You can repeat this in subsequent lines on the screen for up to
15 files. <!--<b>For exercise 1 you only need to submit one file: the text file
containing your answer.</b>--> When you have finished adding files, click
"Confirm".
</li>
<li><b>Choose Action</b> This screen displays the files that you have 
selected to submit. If the list is not correct, click "Pick different
files" and return to step 2. Otherwise, click "Submit". If you receive a 
message that you are not registered for the module, double-check that
you have entered the correct 6-digit student ID number. If you are
sure it is correct, click "Submit anyway", then email Eike Ritter
(<a href="mailto:E.Ritter@cs.bham.ac.uk">E.Ritter@cs.bham.ac.uk</a>) and report 
the problem.
</li>
<li><b>Submit Solution</b> This is the final confirmation before you 
submit your work. If you wish to continue, click "Submit", otherwise
click "Cancel". Note that you can re-submit as many times as you wish 
before the deadline, but that each re-submission overwrites the 
previous one.
</li>
</ol>

A confirmation message will be displayed, indicating that your submission was
successful. You will also be sent an email receipt from BOSS as evidence 
of your submission - you should keep this. You can now log out of BOSS by
clicking "Logout" on the left  of the screen.



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