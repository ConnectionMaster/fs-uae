#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <Wingdi.h>
#include <winspool.h>
#include <winuser.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <commdlg.h>
#include <dlgs.h>
#include <process.h>
#include <prsht.h>
#include <richedit.h>
#include <shellapi.h>
#include <Shlobj.h>
#include <shlwapi.h>
#include <ddraw.h>
#include <shobjidl.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "resource.h"
#include "registry.h"
#include "win32.h"
#include "win32gui.h"

#define MAX_GUI_FONTS 2
#define DEFAULT_FONTSIZE  8

static double multx, multy;

static TCHAR fontname_gui[32], fontname_list[32];
static int fontsize_gui = DEFAULT_FONTSIZE;
static int fontsize_list = DEFAULT_FONTSIZE;
static int fontstyle_gui = 0;
static int fontstyle_list = 0;
static int fontweight_gui = FW_REGULAR;
static int fontweight_list = FW_REGULAR;

static int listviews[16];
static int listviewcnt;
static HFONT listviewfont;
static const TCHAR *fontprefix;

#include <pshpack2.h>
typedef struct {
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;
	/*
	sz_Or_Ord menu;
	sz_Or_Ord windowClass;
	WCHAR title[titleLen];
	*/
} DLGTEMPLATEEX;

typedef struct {
	WORD pointsize;
	WORD weight;
	BYTE italic;
	BYTE charset;
	WCHAR typeface[1];
} DLGTEMPLATEEX_END;

typedef struct {
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	short x;
	short y;
	short cx;
	short cy;
	WORD id;
	WORD reserved;
	WCHAR windowClass[1];
	/* variable data after this */
	/* sz_Or_Ord title; */
	/* WORD extraCount; */
} DLGITEMTEMPLATEEX;
#include <poppack.h>

static int font_vista_ok;
static const wchar_t wfont_vista[] = _T("Segoe UI");
static const wchar_t wfont_xp[] = _T("Tahoma");
static const wchar_t wfont_old[] = _T("MS Sans Serif");
static const TCHAR font_vista[] = _T("Segoe UI");
static const TCHAR font_xp[] = _T("Tahoma");

static int align (double f)
{
	int v = (int)(f + 0.5);
	return v;
}

static int mmx (int v)
{
	return align ((v * multx) / 100.0 + 0.5);
}
static int mmy (int v)
{
	return align ((v * multy) / 100.0 + 0.5);
}


static BYTE *skiptextone (BYTE *s)
{
	s -= sizeof (WCHAR);
	if (s[0] == 0xff && s[1] == 0xff) {
		s += 4;
		return s;
	}
	while (s[0] != 0 || s[1] != 0)
		s += 2;
	s += 2;
	return s;
}

static BYTE *skiptext (BYTE *s)
{
	if (s[0] == 0xff && s[1] == 0xff) {
		s += 4;
		return s;
	}
	while (s[0] != 0 || s[1] != 0)
		s += 2;
	s += 2;
	return s;
}

static BYTE *todword (BYTE *p)
{
	while ((LONG_PTR)p & 3)
		p++;
	return p;
}

static void modifytemplate (DLGTEMPLATEEX *d, DLGTEMPLATEEX_END *d2, int id)
{
	d->cx = mmx (d->cx);
	d->cy = mmy (d->cy);
}

static void modifytemplatefont (DLGTEMPLATEEX *d, DLGTEMPLATEEX_END *d2)
{
	if (!wcscmp (d2->typeface, wfont_old)) {
		wcscpy (d2->typeface, fontname_gui);
		d2->pointsize = fontsize_gui;
		d2->italic = (fontstyle_gui & ITALIC_FONTTYPE) != 0;
		d2->weight = fontweight_gui;
}
}

static void modifyitem (DLGTEMPLATEEX *d, DLGTEMPLATEEX_END *d2, DLGITEMTEMPLATEEX *dt, int id)
{
	bool noyscale = false;

	if (dt->windowClass[0] != 0xffff && (!_tcsicmp (dt->windowClass, WC_LISTVIEWW) || !_tcsicmp (dt->windowClass, WC_TREEVIEWW)))
		listviews[listviewcnt++] = dt->id;

	if (multy >= 89 && multy <= 111) {
		int wc = 0;

		if (dt->windowClass[0] == 0xffff)
			wc = dt->windowClass[1];

		if (wc == 0x0080 && dt->cy <= 20) // button
			noyscale = true;
		if (wc == 0x0085) // checkbox
			noyscale = true;
		if (wc == 0x0081 && dt->cy <= 20) // edit box
			noyscale = true;
	}

	if (!noyscale)
		dt->cy = mmy (dt->cy);

	dt->cx = mmx (dt->cx);
	dt->y = mmy (dt->y);
	dt->x = mmx (dt->x);
}

static INT_PTR CALLBACK DummyProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_DESTROY:
		PostQuitMessage (0);
		return TRUE;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return TRUE;
	case WM_INITDIALOG:
		return TRUE;
	}
	return FALSE;
}

struct newresource *scaleresource (struct newresource *res, HWND parent, int resize)
{
	DLGTEMPLATEEX *d, *s;
	DLGTEMPLATEEX_END *d2, *s2;
	DLGITEMTEMPLATEEX *dt;
	BYTE *p, *p2, *ps, *ps2;
	int i;
	struct newresource *ns;

	listviewcnt = 0;

	d = (DLGTEMPLATEEX*)res->resource;
	d2 = (DLGTEMPLATEEX_END*)res->resource;

	if (d->dlgVer != 1 || d->signature != 0xffff)
		return 0;
	if (!(d->style & (DS_SETFONT | DS_SHELLFONT)))
		return 0;

	ns = xcalloc (struct newresource, 1);
	ns->inst = res->inst;
	ns->size = res->size;
	ns->tmpl = res->tmpl;
	ns->resource = (LPCDLGTEMPLATEW)xmalloc (uae_u8, ns->size + 32);
	memcpy ((void*)ns->resource, res->resource, ns->size);

	d = (DLGTEMPLATEEX*)ns->resource;
	s = (DLGTEMPLATEEX*)res->resource;

	if (resize > 0) {
		d->style &= ~DS_MODALFRAME;
		d->style |= WS_THICKFRAME;
	} else if (resize == 0) {
		d->style |= DS_MODALFRAME;
		d->style &= ~WS_THICKFRAME;
	}

	d2 = (DLGTEMPLATEEX_END*)ns->resource;
	p = (BYTE*)d + sizeof (DLGTEMPLATEEX);
	p = skiptext (p);
	p = skiptext (p);
	p = skiptext (p);

	s2 = (DLGTEMPLATEEX_END*)res->resource;
	ps = (BYTE*)s2 + sizeof (DLGTEMPLATEEX);
	ps = skiptext (ps);
	ps = skiptext (ps);
	ps = skiptext (ps);

	d2 = (DLGTEMPLATEEX_END*)p;
	p2 = p;
	p2 += sizeof (DLGTEMPLATEEX_END);
	p2 = skiptextone (p2);
	p2 = todword (p2);

	s2 = (DLGTEMPLATEEX_END*)ps;
	ps2 = ps;
	ps2 += sizeof (DLGTEMPLATEEX_END);
	ps2 = skiptextone (ps2);
	ps2 = todword (ps2);

	modifytemplatefont (d, d2);

	p += sizeof (DLGTEMPLATEEX_END);
	p = skiptextone (p);
	p = todword (p);

	memcpy (p, ps2, ns->size - (ps2 - (BYTE*)res->resource));

	modifytemplate(d, d2, ns->tmpl);

	for (i = 0; i < d->cDlgItems; i++) {
		dt = (DLGITEMTEMPLATEEX*)p;
		modifyitem (d, d2, dt, ns->tmpl);
		p += sizeof (DLGITEMTEMPLATEEX);
		p = skiptextone (p);
		p = skiptext (p);
		p += ((WORD*)p)[0];
		p += sizeof (WORD);
		p = todword (p);
	}

	ns->width = d->cx;
	ns->height = d->cy;
	return ns;
}

void freescaleresource (struct newresource *ns)
{
	xfree ((void*)ns->resource);
	xfree (ns);
}

static void openfont (bool force)
{
	HDC hdc;
	int size;

	if (listviewfont && !force)
		return;
	if (listviewfont)
		DeleteObject (listviewfont);

	hdc = GetDC (NULL);

	size = -MulDiv (fontsize_list, GetDeviceCaps (hdc, LOGPIXELSY), 72);
	listviewfont = CreateFont (size, 0, 0, 0, fontweight_list, (fontstyle_list & ITALIC_FONTTYPE) != 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontname_list);

	ReleaseDC (NULL, hdc);
}

void scaleresource_setfont (HWND hDlg)
{
	if (!listviewcnt || (!_tcscmp (fontname_gui, fontname_list) && fontsize_gui == fontsize_list && fontstyle_gui == fontstyle_list && fontweight_gui == fontweight_list))
		return;
	openfont (false);
	if (!listviewfont)
		return;
	for (int i = 0; i < listviewcnt; i++) {
		SendMessage (GetDlgItem (hDlg, listviews[i]), WM_SETFONT, WPARAM(listviewfont), FALSE);
	}
}

static void setdeffont (void)
{
	_tcscpy (fontname_gui, font_vista_ok ? wfont_vista : wfont_xp);
	fontsize_gui = DEFAULT_FONTSIZE;
	fontstyle_gui = 0;
	fontweight_gui = FW_REGULAR;
	_tcscpy (fontname_list, font_vista_ok ? wfont_vista : wfont_xp);
	fontsize_list = DEFAULT_FONTSIZE;
	fontstyle_list = 0;
	fontweight_list = FW_REGULAR;
}

static TCHAR *fontreg[2] = { _T("GUIFont"), _T("GUIListFont") };

static void regsetfont (UAEREG *reg, const TCHAR *prefix, const TCHAR *name, const TCHAR *fontname, int fontsize, int fontstyle, int fontweight)
{
	TCHAR tmp[256], tmp2[256];

	_stprintf (tmp, _T("%s:%d:%d:%d"), fontname, fontsize, fontstyle, fontweight);
	_stprintf (tmp2, _T("%s%s"), name, prefix);
	regsetstr (reg, tmp2, tmp);
}
static void regqueryfont (UAEREG *reg, const TCHAR *prefix, const TCHAR *name, TCHAR *fontname, int *pfontsize, int *pfontstyle, int *pfontweight)
{
	TCHAR tmp2[256], tmp[256], *p1, *p2, *p3, *p4;
	int size;
	int fontsize, fontstyle, fontweight;

	_tcscpy (tmp2, name);
	_tcscat (tmp2, prefix);
	size = sizeof tmp / sizeof (TCHAR);
	if (!regquerystr (reg, tmp2, tmp, &size))
		return;
	p1 = _tcschr (tmp, ':');
	if (!p1)
		return;
	*p1++ = 0;
	p2 = _tcschr (p1, ':');
	if (!p2)
		return;
	*p2++ = 0;
	p3 = _tcschr (p2, ':');
	if (!p3)
		return;
	*p3++ = 0;
	p4 = _tcschr (p3, ':');
	if (p4)
		*p4 = 0;

	_tcscpy (fontname, tmp);
	fontsize = _tstoi (p1);
	fontstyle = _tstoi (p2);
	fontweight = _tstoi (p3);

	if (fontsize == 0)
		fontsize = 8;
	if (fontsize < 5)
		fontsize = 5;
	if (fontsize > 20)
		fontsize = 20;
	*pfontsize = fontsize;

	*pfontstyle = fontstyle;

	*pfontweight = fontweight;
}

void scaleresource_setdefaults (void)
{
	setdeffont ();
	for (int i = 0; i < MAX_GUI_FONTS; i++) {
		TCHAR tmp[256];
		_stprintf (tmp, _T("%s%s"), fontreg[i], fontprefix);
		regdelete (NULL, tmp);
	}
	openfont (true);
}

static int lpx, lpy;

double scaleresource_getdpimult (void)
{
	HDC hdc = GetDC (NULL);
	int m = GetDeviceCaps (hdc, LOGPIXELSY);
	ReleaseDC (NULL, hdc);
	if (m > 96) {
		m -= 96;
		return 1.0 + m / 250.0;
	}
	return 1.0;
}

void scaleresource_init (const TCHAR *prefix)
{
	if (os_vista)
		font_vista_ok = 1;

	fontprefix = prefix;

	HDC hdc = GetDC (NULL);
	lpx = GetDeviceCaps (hdc, LOGPIXELSX);
	lpy = GetDeviceCaps (hdc, LOGPIXELSY);
	ReleaseDC (NULL, hdc);

	setdeffont ();

	regqueryfont (NULL, fontprefix, fontreg[0], fontname_gui, &fontsize_gui, &fontstyle_gui, &fontweight_gui);
	regqueryfont (NULL, fontprefix, fontreg[1], fontname_list, &fontsize_list, &fontstyle_list, &fontweight_list);

	//write_log (_T("GUI font %s:%d:%d:%d\n"), fontname_gui, fontsize_gui, fontstyle_gui, fontweight_gui);
	//write_log (_T("List font %s:%d:%d:%d\n"), fontname_list, fontsize_list, fontstyle_list, fontweight_list);

	openfont (true);
}

static void sizefont (HWND hDlg, const TCHAR *name, int size, int style, int weight, int *width, int *height)
{
	/* ARGH!!! */

	HDC hdc = GetDC (hDlg);
	size = -MulDiv (size, lpy, 72);
	HFONT font = CreateFont (size, 0, 0, 0, weight,
		(style & ITALIC_FONTTYPE) != 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name);
	if (!font) {
		*width = 8;
		*height = 8;
	} else {
		HFONT hFontOld = (HFONT)SelectObject (hdc, font);
		TEXTMETRIC tm;
		SIZE fsize;
		GetTextMetrics (hdc, &tm);
		GetTextExtentPoint32 (hdc, _T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"), 52, &fsize);
		*width = (fsize.cx / 26 + 1) / 2;
		*height = tm.tmHeight;
		SelectObject (hdc, hFontOld);
		DeleteObject (font);
	}
	ReleaseDC (hDlg, hdc);
}

void scaleresource_setmult (HWND hDlg, int w, int h)
{
	int width, height, width2, height2;
	
	if (w < 0) {
		multx = -w;
		multy = -h;
		return;
	}

	int cy = GetSystemMetrics (SM_CYSIZEFRAME);
	int cx = GetSystemMetrics (SM_CXSIZEFRAME);
	int caption = GetSystemMetrics (SM_CYCAPTION);

	sizefont (hDlg, fontname_gui, fontsize_gui, fontstyle_gui, fontweight_gui, &width, &height);
	sizefont (hDlg, wfont_old, 8, REGULAR_FONTTYPE, FW_REGULAR, &width2, &height2);

	int yy = cy * 2 + caption;
	int xx = cx * 2;

	w += xx;
	h += yy;

	multx = (w * width2) * 100.0 / (GUI_INTERNAL_WIDTH * width);
	multy = (h * height2) * 100.0 / (GUI_INTERNAL_HEIGHT * height);

	multx = MulDiv (multx, 96, lpx);
	multy = MulDiv (multy, 96, lpy);

	if (multx < 50)
		multx = 50;
	if (multy < 50)
		multy = 50;

	//write_log (_T("MX=%f MY=%f\n"), multx, multy);
}

void scaleresource_getmult (int *mx, int *my)
{
	if (mx)
		*mx = (int)(multx + 0.5);
	if (my)
		*my = (int)(multy + 0.5);
}


int scaleresource_choosefont (HWND hDlg, int fonttype)
{
	CHOOSEFONT cf = { 0 };
	LOGFONT lf = { 0 };
	HDC hdc;
	TCHAR *fontname[2];
	int *fontsize[2], *fontstyle[2], *fontweight[2];
	int lm;

	fontname[0] = fontname_gui;
	fontname[1] = fontname_list;
	fontsize[0] = &fontsize_gui;
	fontsize[1] = &fontsize_list;
	fontstyle[0] = &fontstyle_gui;
	fontstyle[1] = &fontstyle_list;
	fontweight[0] = &fontweight_gui;
	fontweight[1] = &fontweight_list;

	cf.lStructSize = sizeof cf;
	cf.hwndOwner = hDlg;
	cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL | CF_SCREENFONTS;
	cf.lpLogFont = &lf;
	cf.nFontType = REGULAR_FONTTYPE;
	cf.iPointSize = *fontsize[fonttype];

	hdc = GetDC (NULL);
	lm = GetDeviceCaps (hdc, LOGPIXELSY);

	_tcscpy (lf.lfFaceName, fontname[fonttype]);
	lf.lfHeight = -MulDiv (*fontsize[fonttype], lm, 72);
	lf.lfWeight = *fontweight[fonttype];
	lf.lfItalic = (*fontstyle[fonttype] & ITALIC_FONTTYPE) != 0;

	if (!ChooseFont (&cf)) {
		ReleaseDC (NULL, hdc);
		return 0;
	}

	_tcscpy (fontname[fonttype], lf.lfFaceName);
	*fontsize[fonttype] = lf.lfHeight;
	*fontsize[fonttype] = -MulDiv (*fontsize[fonttype], 72, GetDeviceCaps (hdc, LOGPIXELSY));

	*fontstyle[fonttype] = lf.lfItalic ? ITALIC_FONTTYPE : 0;

	*fontweight[fonttype] = lf.lfWeight;

	ReleaseDC (NULL, hdc);

	regsetfont (NULL, fontprefix, fontreg[fonttype], fontname[fonttype], *fontsize[fonttype], *fontstyle[fonttype], *fontweight[fonttype]);

	openfont (true);


	return 1;
}

