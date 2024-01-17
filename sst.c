
/******************************************************************************
 * 
 * Copyright (C) 2023 github.com/AlexanderCharles
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 3 for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 *****************************************************************************/

/******************************************************************************
 * 
 * What is this program?
 *	A simple screenshot/snippet tool.
 * 
 * What can this program do?
 *	It takes screenshots, saves them to /tmp/ and copies the latest one.
 * 
 * What is the goal of this program?
 * 	To create a basic screenshot tool. The code is designed to be simple.
 * 
 * How do I use this program?
 * 	You can run it through the command line or bind the program to a key.
 * 
 * How do I compile and install this program?
 * 	Build with 'make', install with 'make install'.
 * 
 * What problems does this software have?
 * 	1. I hasn't been tested on many systems.
 * 	2. It has not been tested with multiple monitors, and likely won't work.
 * 	3. It must be ran with MB1 released to generate the initial position.
 * 	4. It spams the /tmp/ folder.
 * 
 *****************************************************************************/



#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include <uuid/uuid.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "config.h"



typedef struct Screenshot {
	struct {
		unsigned char r, g, b;
	} *data;
	size_t data_length;
	unsigned int width, height;
	unsigned char channels;
} Screenshot;



static void
GetMousePosition(Display *i_d, Window *i_w,
                 int *o_x, int *o_y)
{
	int winX, winY;
	unsigned int mask;
	XQueryPointer(i_d,
	              DefaultRootWindow(i_d),
	              i_w,   i_w,
	              o_x,   o_y,
	              &winX, &winY,
	              &mask);
}

static void
Fullscreen(Display *i_d, Window i_w)
{
	Atom atoms[1] = {
		XInternAtom(i_d, "_NET_WM_STATE_FULLSCREEN", False)
	};
	XChangeProperty(
		i_d, i_w,
		XInternAtom(i_d, "_NET_WM_STATE", False),
		XA_ATOM, 32, PropModeReplace,
		(unsigned char*) atoms, 1
	);
	XFlush(i_d);
}

static Screenshot
CreateScreenshot(Display *i_d, Drawable i_drw,
                 int i_x, int i_y, int i_w, int i_h)
{
	Screenshot result;
	XImage    *img;
	int        channelCount;
	
	result = (Screenshot) { 0 };
	
	img = XGetImage(i_d, i_drw, i_x, i_y, i_w, i_h, AllPlanes, ZPixmap);
	if (img == NULL) {
		printf("ERROR: Could not create XImage\n");
		exit(EXIT_FAILURE);
	}
	
	channelCount = (img->bits_per_pixel / 8);
	result.channels    = 3;
	result.width       = img->width;
	result.height      = img->height;
	result.data_length = result.width * result.height;
	result.data        = malloc(
		(sizeof(unsigned char) * result.channels) * result.data_length
	);
	
	for (unsigned int i = 0; i < result.data_length; ++i) {
		result.data[i].r = img->data[i * channelCount + 2];
		result.data[i].g = img->data[i * channelCount + 1];
		result.data[i].b = img->data[i * channelCount + 0];
	}
	
	XDestroyImage(img);
	return(result);
}



int
main(void)
{
	Display             *display;
	Window               window;
	Window               overlay;
	int                  screen;
	Visual              *visual;
	int                  depth;
	XWindowAttributes    winAttribs;
	XSetWindowAttributes setWinAttribs;
	
	int   run;
	XEvent event;
	
	int mousePosX, startPosX, currPosX;
	int mousePosY, startPosY, currPosY;
	
	XGCValues GCValues;
	GC        pixelSwapGC;
	Pixmap    backgroundPixmap;
	
	GC        drawGC;
	XColor    drawColour;
	
	startPosX = startPosY = -1;
	currPosX  = currPosY  = -1;
	
	winAttribs       = (XWindowAttributes)    { 0 };
	GCValues         = (XGCValues)            { 0 };
	setWinAttribs    = (XSetWindowAttributes) { 0 };
	backgroundPixmap = (Pixmap)               { 0 };
	
	display = XOpenDisplay(NULL);
	window  = XDefaultRootWindow(display);
	screen  = XDefaultScreen(display);
	visual  = DefaultVisual(display, screen);
	depth   = DefaultDepth(display, screen);
	XGetWindowAttributes(display, window, &winAttribs);
	
	XGrabButton(display, AnyButton, AnyModifier, window, 0,
	            ButtonPressMask | Button1MotionMask | ButtonReleaseMask,
	            GrabModeAsync, GrabModeAsync, None, None);
	
	GCValues.function   = GXcopy;
	GCValues.plane_mask = AllPlanes;
	if ((pixelSwapGC = XCreateGC(display, window,
	                             GCFunction | GCPlaneMask,
	                             &GCValues)) == NULL) {
		printf("ERROR: Could not create pixelSwapGC\n");
		exit(EXIT_FAILURE);
	}
	
	setWinAttribs.colormap = DefaultColormap(display, screen);
	drawColour.red   = SC_RED   * 65535 / 255;
	drawColour.green = SC_GREEN * 65535 / 255;
	drawColour.blue  = SC_BLUE  * 65535 / 255;
	drawColour.flags = DoRed | DoGreen | DoBlue;
	if (!XAllocColor(display, setWinAttribs.colormap, &drawColour)) {
		printf("ERROR: Could not create colour\n");
		exit(EXIT_FAILURE);
	}
	
	GCValues = (XGCValues) { 0 };
	GCValues.line_width = line_width;
	GCValues.line_style = LineSolid;
	GCValues.cap_style  = CapButt;
	GCValues.join_style = JoinBevel;
	GCValues.foreground = drawColour.pixel;
	if ((drawGC = XCreateGC(
		display, window,
		GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle | GCForeground,
		&GCValues
	)) == NULL) {
		printf("ERROR: Could not create drawGC\n");
		exit(EXIT_FAILURE);
	}
	
	setWinAttribs.override_redirect = True;
	overlay =
		XCreateWindow(display, DefaultRootWindow(display), 0, 0,
		              winAttribs.width, winAttribs.height, 0,
		              depth, InputOutput, visual,
		              CWOverrideRedirect,
		              &setWinAttribs);
	XMapWindow(display, overlay);
	XFlush(display);
	
	backgroundPixmap = XCreatePixmap(
		display, window,
		winAttribs.width, winAttribs.height,
		depth
	);
	XCopyArea(display, overlay, backgroundPixmap, pixelSwapGC,
	          0, 0, winAttribs.width, winAttribs.height, 0, 0);
	
	XMapWindow(display, overlay);
	Fullscreen(display, overlay);
	
	for (run = True; run == True;) {
		XNextEvent(display, &event);
		
		GetMousePosition(display, &window, &mousePosX, &mousePosY);
		
		switch (event.type) {
			case MotionNotify:
			{
				if (startPosX != -1) {
					int originX, originY;
					
					originX = startPosX;
					originY = startPosY;
					currPosX = mousePosX;
					currPosY = mousePosY;
					
					if (startPosX > currPosX) {
						currPosX  = originX;
						originX   = mousePosX;
					}
					if (startPosY > currPosY) {
						currPosY = originY;
						originY  = mousePosY;
					}
					
					XClearWindow(display, window);
					XClearWindow(display, overlay);
					XCopyArea(display, backgroundPixmap, overlay, pixelSwapGC,
					          0, 0, winAttribs.width, winAttribs.height, 0, 0);
					
					XDrawRectangle(display, overlay, drawGC,
					               originX, originY,
					               currPosX - originX,
					               currPosY - originY);
					
					XFlush(display);
				}
			}
			break;
			case ButtonPress:
			{
				if (event.xbutton.button == Button1) {
					startPosX = mousePosX;
					startPosY = mousePosY;
				}
			}
			break;
			case ButtonRelease:
			{
				if (event.xbutton.button == Button1) {
					if (!(startPosX == -1 || currPosX == -1 ||
					    (currPosX - startPosX) < 1 || (currPosY - startPosY) < 1)) {
						char   fname[64]   = { 0 };
						char   UID[48]     = { 0 };
						char   buffer[128] = { 0 };
						
						uuid_t genUID;
						int    result;
						
						Screenshot screenshot;
						
						uuid_generate_random(genUID);
						uuid_unparse(genUID, UID);
						sprintf(fname, "%ssst-%s.png", save_dir, UID);
						
						XCopyArea(display, backgroundPixmap, overlay, pixelSwapGC,
						          0, 0, winAttribs.width, winAttribs.height, 0, 0);
						screenshot =
							CreateScreenshot(display, backgroundPixmap,
							                 startPosX, startPosY,
							                 currPosX - startPosX, currPosY - startPosY);
						stbi_write_png(fname, screenshot.width, screenshot.height,
						               screenshot.channels, screenshot.data,
						               screenshot.width * screenshot.channels);
						free(screenshot.data);
						
						sprintf(buffer,
						        "xclip -selection clipboard -t image/png %s",
						        fname);
						result = system(buffer);
						if (result != 0) {
							printf("ERROR: Could not copy image to clipboard\n");
						}
					}
					
					run = False;
					break;
				}
			}
			break;
		}
	}
	
	XUnmapWindow(display, overlay);
	
	XFreePixmap(display, backgroundPixmap);
	XDestroyWindow(display, overlay);
	XCloseDisplay(display);
	
	return 0;
}
