/* modifier 0 means no modifier */
static char *useragent      = "Mozilla/5.0 (X11; U; Unix; en-US) "
	"AppleWebKit/537.15 (KHTML, like Gecko) Chrome/24.0.1295.0 "
	"Safari/537.15 Surf/"VERSION;
static char *scriptfile     = "~/.surf/script.js";
static char *styledir       = "~/.surf/styles/";
static char *cachefolder    = "~/.surf/cache/";

static Bool kioskmode       = FALSE; /* Ignore shortcuts */
static Bool showindicators  = FALSE;  /* Show indicators in window title */
static Bool zoomto96dpi     = TRUE;  /* Zoom pages to always emulate 96dpi */
static Bool runinfullscreen = FALSE; /* Run in fullscreen mode by default */

static guint defaultfontsize = 12;   /* Default font size */
static gfloat zoomlevel = 1.0;       /* Default zoom level */

/* Soup default features */
static char *cookiefile     = "~/.surf/cookies.txt";
static char *cookiepolicies = "Aa@"; /* A: accept all; a: accept nothing,
                                        @: accept no third party */
static char *cafile         = "/etc/ssl/certs/ca-certificates.crt";
static char *strictssl      = FALSE; /* Refuse untrusted SSL connections */
static time_t sessiontime   = 3600;

/* Webkit default features */
static Bool enablescrollbars      = TRUE;
static Bool enablespatialbrowsing = TRUE;
static Bool enablediskcache       = TRUE;
static int diskcachebytes         = 5 * 1024 * 1024;
static Bool enableplugins         = TRUE;
static Bool enablescripts         = TRUE;
static Bool enableinspector       = TRUE;
static Bool enablestyles          = TRUE;
static Bool loadimages            = TRUE;
static Bool hidebackground        = FALSE;
static Bool allowgeolocation      = TRUE;

#define SETPROP(p, q) { \
	.v = (char *[]){ "/bin/sh", "-c", \
		"prop=\"`(xprop -id $2 $0 | cut -d '\"' -f 2 | xargs -0 printf %b && "\
                "cat ~/.surf/bookmarks) | dmenu -l 20`\" &&" \
		"xprop -id $2 -f $1 8s -set $1 \"$prop\"", \
		p, q, winid, NULL \
	} \
}

#define SETPROPFIND(p, q) { \
	.v = (char *[]){ "/bin/sh", "-c", \
        "prop=\"`xprop -id $2 $0 | cut -d '\"' -f 2 | xargs -0 printf %b | dmenu`\" &&" \
		"xprop -id $2 -f $1 8s -set $1 \"$prop\"", \
		p, q, winid, NULL \
	} \
}

/* DOWNLOAD
#define (d, r) { \
	.v = (char *[]){ "/bin/sh", "-c", \
		"terminator -e  \"curl -L -J -O --user-agent '$1'" \
		" --referer '$2' -b $3 -c $3 '$0';" \
		" echo done; echo $0; read;\"", \
		d, useragent, r, cookiefile, NULL \
	} \
}
*/

/* DOWNLOAD(URI, referer) */
#define DOWNLOAD(d, r) { \
    .v = (char *[]){ "/bin/sh", "-c", \
        "terminator -e \"aria2c -U '$1'" \
        " --referer '$2' --load-cookies $3 --save-cookies $3 '$0';" \
        " read;\"", \
        d, useragent, r, cookiefile, NULL \
    } \
}


/* PLUMB(URI) */
/* This called when some URI which does not begin with "about:",
 * "http://" or "https://" should be opened.
 */
#define PLUMB(u) {\
	.v = (char *[]){ "/bin/sh", "-c", \
		"xdg-open \"$0\"", u, NULL \
	} \
}

/* styles */
/*
 * The iteration will stop at the first match, beginning at the beginning of
 * the list.
 */
static SiteStyle styles[] = {
	/* regexp		file in $styledir */
	{ ".*",			"default.css" },
};

static SearchEngine searchengines[] = {
    { "g",   "http://www.google.com/search?q=%s"   },
    { "t", "https://translate.google.com/#auto/sk/%s" },
};


#define BM_ADD { .v = (char *[]){ "/bin/sh", "-c", \
  "(echo `xprop -id $0 _SURF_URI | cut -d '\"' -f 2` && "\
  "cat ~/.surf/bookmarks) | awk '!seen[$0]++' > ~/.surf/bookmarks_new && "\
  "mv ~/.surf/bookmarks_new ~/.surf/bookmarks", \
  winid, NULL } }

#define MODKEY GDK_CONTROL_MASK

/* hotkeys */
/*
 * If you use anything else but MODKEY and GDK_SHIFT_MASK, don't forget to
 * edit the CLEANMASK() macro.
 */
static Key keys[] = {
    /* modifier	            keyval      function    arg             Focus */
    { MODKEY,               GDK_b,      spawn,      BM_ADD },
    { MODKEY|GDK_SHIFT_MASK,GDK_r,      reload,     { .b = TRUE } },
    { MODKEY,               GDK_r,      reload,     { .b = FALSE } },
    { MODKEY|GDK_SHIFT_MASK,GDK_p,      print,      { 0 } },

    { MODKEY,               GDK_p,      clipboard,  { .b = TRUE } },
    { MODKEY,               GDK_y,      clipboard,  { .b = FALSE } },

    { MODKEY|GDK_SHIFT_MASK,GDK_j,      zoom,       { .i = -1 } },
    { MODKEY|GDK_SHIFT_MASK,GDK_k,      zoom,       { .i = +1 } },
    { MODKEY|GDK_SHIFT_MASK,GDK_q,      zoom,       { .i = 0  } },
    { MODKEY,               GDK_minus,  zoom,       { .i = -1 } },
    { MODKEY,               GDK_plus,   zoom,       { .i = +1 } },

    { MODKEY|GDK_SHIFT_MASK, GDK_l,      navigate,   { .i = +1 } },
    { MODKEY|GDK_SHIFT_MASK, GDK_h,      navigate,   { .i = -1 } },

    { MODKEY,               GDK_j,      scroll_v,   { .i = +1 } },
    { MODKEY,               GDK_k,      scroll_v,   { .i = -1 } },
    { MODKEY,               GDK_b,      scroll_v,   { .i = -10000 } },
    { MODKEY,               GDK_space,  scroll_v,   { .i = +10000 } },
    { MODKEY,               GDK_i,      scroll_h,   { .i = +1 } },
    { MODKEY,               GDK_u,      scroll_h,   { .i = -1 } },

    { 0,                    GDK_F11,    fullscreen, { 0 } },
    { 0,                    GDK_Escape, stop,       { 0 } },
    { MODKEY,               GDK_o,      source,     { 0 } },
    { MODKEY|GDK_SHIFT_MASK,GDK_o,      inspector,  { 0 } },

    { MODKEY,               GDK_g,      spawn,      SETPROP("_SURF_URI", "_SURF_GO") },
    { MODKEY,               GDK_f,      spawn,      SETPROPFIND("_SURF_FIND", "_SURF_FIND") },
    { MODKEY,               GDK_slash,  spawn,      SETPROPFIND("_SURF_FIND", "_SURF_FIND") },

    { MODKEY,               GDK_n,      find,       { .b = TRUE } },
    { MODKEY|GDK_SHIFT_MASK,GDK_n,      find,       { .b = FALSE } },

    { MODKEY|GDK_SHIFT_MASK,GDK_c,      toggle,     { .v = "enable-caret-browsing" } },
    { MODKEY|GDK_SHIFT_MASK,GDK_i,      toggle,     { .v = "auto-load-images" } },
    { MODKEY|GDK_SHIFT_MASK,GDK_s,      toggle,     { .v = "enable-scripts" } },
    { MODKEY|GDK_SHIFT_MASK,GDK_v,      toggle,     { .v = "enable-plugins" } },
    { MODKEY|GDK_SHIFT_MASK,GDK_a,      togglecookiepolicy, { 0 } },
    { MODKEY|GDK_SHIFT_MASK,GDK_m,      togglestyle, { 0 } },
    { MODKEY|GDK_SHIFT_MASK,GDK_b,      togglescrollbars, { 0 } },
    { MODKEY|GDK_SHIFT_MASK,GDK_g,      togglegeolocation, { 0 } },
};

/* button definitions */
/* click can be ClkDoc, ClkLink, ClkImg, ClkMedia, ClkSel, ClkEdit, ClkAny */
static Button buttons[] = {
    /* click                event mask  button  function        argument */
    { ClkLink,              0,          2,      linkopenembed,  { 0 } },
    { ClkLink,              MODKEY,     2,      linkopen,       { 0 } },
    { ClkLink,              MODKEY,     1,      linkopenembed,       { 0 } },
    { ClkAny,               0,          8,      navigate,       { .i = -1 } },
    { ClkAny,               0,          9,      navigate,       { .i = +1 } },
};
