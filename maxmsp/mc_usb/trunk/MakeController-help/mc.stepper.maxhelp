{
	"patcher" : 	{
		"fileversion" : 1,
		"rect" : [ 126.0, 160.0, 975.0, 504.0 ],
		"bglocked" : 0,
		"defrect" : [ 126.0, 160.0, 975.0, 504.0 ],
		"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 0,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 0,
		"toolbarvisible" : 1,
		"boxanimatetime" : 200,
		"imprint" : 0,
		"metadata" : [  ],
		"boxes" : [ 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "step 500",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-62",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 381.0, 249.0, 57.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "step 500",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-56",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 148.0, 249.0, 57.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Only one mc.usb object is required/allowed per patch - use the send (s) and receive (r) objects to pass info to and from it.",
					"linecount" : 4,
					"fontsize" : 12.0,
					"id" : "obj-57",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 531.0, 404.0, 192.0, 62.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "r to-mc-usb",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-60",
					"numinlets" : 0,
					"fontname" : "Arial",
					"patching_rect" : [ 737.0, 399.0, 71.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "s from-mc-usb",
					"fontsize" : 12.0,
					"id" : "obj-59",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 737.0, 450.0, 87.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "mc.usb",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-58",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 737.0, 425.0, 51.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"bgcolor" : [ 0.807843, 0.807843, 0.807843, 1.0 ],
					"id" : "obj-61",
					"numinlets" : 1,
					"patching_rect" : [ 727.0, 387.0, 110.0, 97.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Stepper Motor How-To",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-54",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 658.0, 185.0, 132.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "t b",
					"outlettype" : [ "bang" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-52",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 864.0, 238.0, 22.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : ";\rmax launch_browser http://www.makingthings.com/documentation/how-to/stepper-motor",
					"linecount" : 3,
					"outlettype" : [ "" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-53",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 1012.0, 393.0, 366.0, 46.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "set 1",
					"outlettype" : [ "" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-1",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 354.0, 143.0, 42.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "loadbang",
					"outlettype" : [ "bang" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-2",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 354.0, 120.0, 61.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "• use the positionrequested property to set where you'd like the stepper to be, and you can then read back how far along the stepper is towards that destination by sending the position message.",
					"linecount" : 4,
					"fontsize" : 12.0,
					"id" : "obj-3",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 627.0, 298.0, 281.0, 62.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "• halfstep sets whether to move in half-step increments or not.",
					"fontsize" : 12.0,
					"id" : "obj-4",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 626.0, 273.0, 337.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "• bipolar sets whether the stepper motor is bipolar or not.",
					"fontsize" : 12.0,
					"id" : "obj-5",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 626.0, 254.0, 338.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "• duty sets how much power is getting to the stepper.",
					"fontsize" : 12.0,
					"id" : "obj-6",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 626.0, 235.0, 338.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "• speed sets the number of milliseconds between steps.",
					"fontsize" : 12.0,
					"id" : "obj-7",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 626.0, 216.0, 337.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-8",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 395.0, 85.0, 37.0, 20.0 ],
					"numoutlets" : 2,
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"fontsize" : 12.0,
					"minimum" : 0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 71000,
					"id" : "obj-9",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 463.0, 107.0, 37.0, 20.0 ],
					"numoutlets" : 2,
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"fontsize" : 12.0,
					"minimum" : 0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 1023,
					"id" : "obj-10",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 475.0, 151.0, 37.0, 20.0 ],
					"numoutlets" : 2,
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-11",
					"numinlets" : 1,
					"patching_rect" : [ 493.0, 253.0, 25.0, 25.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-12",
					"numinlets" : 1,
					"patching_rect" : [ 486.0, 200.0, 25.0, 25.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "halfstep $1",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-13",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 493.0, 284.0, 78.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "bipolar $1",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-14",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 486.0, 230.0, 72.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "duty $1",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-15",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 475.0, 174.0, 57.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "position",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-16",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 449.0, 86.0, 57.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "speed $1",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-17",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 463.0, 130.0, 64.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "positionrequested",
					"fontsize" : 12.0,
					"id" : "obj-18",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 374.0, 66.0, 120.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-19",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 206.0, 85.0, 37.0, 20.0 ],
					"numoutlets" : 2,
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"fontsize" : 12.0,
					"minimum" : 0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 71000,
					"id" : "obj-20",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 274.0, 107.0, 37.0, 20.0 ],
					"numoutlets" : 2,
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"fontsize" : 12.0,
					"minimum" : 0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 1023,
					"id" : "obj-21",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 286.0, 151.0, 37.0, 20.0 ],
					"numoutlets" : 2,
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-22",
					"numinlets" : 1,
					"patching_rect" : [ 304.0, 253.0, 25.0, 25.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-23",
					"numinlets" : 1,
					"patching_rect" : [ 297.0, 198.0, 25.0, 25.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "halfstep $1",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-24",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 304.0, 284.0, 78.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "bipolar $1",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-25",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 297.0, 228.0, 72.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "duty $1",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-26",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 286.0, 174.0, 57.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "position",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-27",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 260.0, 86.0, 57.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "mc.stepper accepts two kinds of input: integers and messages. \n\nintegers are used to set the position of the stepper motor and a variety of other messages are used to configure/use the stepper motor.",
					"linecount" : 8,
					"fontsize" : 12.0,
					"id" : "obj-28",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 18.0, 109.0, 190.0, 117.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "speed $1",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-29",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 274.0, 130.0, 64.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "mc.stepper",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-30",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 204.0, 310.0, 256.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "positionrequested",
					"fontsize" : 12.0,
					"id" : "obj-31",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 178.0, 66.0, 120.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "mc.stepper",
					"fontsize" : 18.0,
					"frgb" : [ 0.082353, 0.219608, 0.035294, 1.0 ],
					"id" : "obj-33",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 12.0, 11.0, 126.0, 27.0 ],
					"numoutlets" : 0,
					"textcolor" : [ 0.082353, 0.219608, 0.035294, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Control stepper motors with the Make Controller Kit.",
					"fontsize" : 12.0,
					"id" : "obj-34",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 12.0, 36.0, 386.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"bgcolor" : [ 0.905882, 0.905882, 0.905882, 1.0 ],
					"id" : "obj-35",
					"numinlets" : 1,
					"patching_rect" : [ 7.0, 6.0, 623.0, 58.0 ],
					"numoutlets" : 0,
					"rounded" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Click on the gate to switch whether the messages get sent via Ethernet or USB. Ethernet is on the left, USB on the right.",
					"linecount" : 3,
					"fontsize" : 12.0,
					"id" : "obj-36",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 358.0, 345.0, 260.0, 48.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "s to-mc-usb",
					"fontsize" : 12.0,
					"id" : "obj-37",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 456.0, 428.0, 73.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "gswitch2",
					"outlettype" : [ "", "" ],
					"id" : "obj-38",
					"numinlets" : 2,
					"patching_rect" : [ 314.0, 350.0, 39.0, 32.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "t b",
					"outlettype" : [ "bang" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-39",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 882.0, 187.0, 22.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "t b",
					"outlettype" : [ "bang" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-40",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 899.0, 163.0, 22.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "t b",
					"outlettype" : [ "bang" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-41",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 916.0, 142.0, 22.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : ";\rmax launch_browser http://www.makingthings.com/forum",
					"linecount" : 2,
					"outlettype" : [ "" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-42",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 1030.0, 342.0, 370.0, 32.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : ";\rmax launch_browser http://www.makingthings.com/ref/firmware/html/group___o_s_c.html",
					"linecount" : 3,
					"outlettype" : [ "" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-43",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 1050.0, 260.0, 369.0, 46.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : ";\rmax launch_browser http://www.makingthings.com/documentation/tutorial/max-msp",
					"linecount" : 2,
					"outlettype" : [ "" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-44",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 1068.0, 202.0, 455.0, 32.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Make Controller Kit Discussion Forum",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-45",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 658.0, 163.0, 235.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Make Controller Kit OSC Reference",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-46",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 658.0, 142.0, 219.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Make Controller Max/MSP Tutorial",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-47",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 658.0, 122.0, 223.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Check the MakingThings website for more info and reference material:",
					"linecount" : 2,
					"fontsize" : 12.0,
					"id" : "obj-48",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 659.0, 95.0, 221.0, 34.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "<- Adjust this to the address of your board.",
					"linecount" : 2,
					"fontsize" : 12.0,
					"id" : "obj-49",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 303.0, 422.0, 146.0, 34.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "fpic",
					"id" : "obj-50",
					"numinlets" : 1,
					"patching_rect" : [ 639.0, 10.0, 226.0, 82.0 ],
					"pic" : "logo.png",
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "udpsend 192.168.0.200 10000",
					"fontsize" : 12.0,
					"id" : "obj-51",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 123.0, 427.0, 175.0, 20.0 ],
					"numoutlets" : 0
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-62", 0 ],
					"destination" : [ "obj-30", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-56", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-13", 0 ],
					"destination" : [ "obj-30", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 0 ],
					"destination" : [ "obj-30", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-15", 0 ],
					"destination" : [ "obj-30", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-17", 0 ],
					"destination" : [ "obj-30", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-16", 0 ],
					"destination" : [ "obj-30", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-8", 0 ],
					"destination" : [ "obj-30", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-30", 0 ],
					"destination" : [ "obj-38", 1 ],
					"hidden" : 0,
					"midpoints" : [ 213.5, 340.0, 343.5, 340.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-27", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-29", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-26", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-25", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-24", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-19", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-38", 0 ],
					"destination" : [ "obj-51", 0 ],
					"hidden" : 0,
					"midpoints" : [ 323.5, 405.0, 132.5, 405.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-20", 0 ],
					"destination" : [ "obj-29", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-21", 0 ],
					"destination" : [ "obj-26", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 0 ],
					"destination" : [ "obj-23", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-23", 0 ],
					"destination" : [ "obj-25", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-22", 0 ],
					"destination" : [ "obj-24", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-2", 0 ],
					"destination" : [ "obj-1", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-9", 0 ],
					"destination" : [ "obj-17", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-10", 0 ],
					"destination" : [ "obj-15", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 0 ],
					"destination" : [ "obj-12", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-12", 0 ],
					"destination" : [ "obj-14", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-11", 0 ],
					"destination" : [ "obj-13", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-45", 0 ],
					"destination" : [ "obj-39", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-46", 0 ],
					"destination" : [ "obj-40", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-47", 0 ],
					"destination" : [ "obj-41", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-39", 0 ],
					"destination" : [ "obj-42", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-40", 0 ],
					"destination" : [ "obj-43", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-41", 0 ],
					"destination" : [ "obj-44", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-52", 0 ],
					"destination" : [ "obj-53", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-54", 0 ],
					"destination" : [ "obj-52", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-60", 0 ],
					"destination" : [ "obj-58", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-58", 0 ],
					"destination" : [ "obj-59", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-38", 1 ],
					"destination" : [ "obj-37", 0 ],
					"hidden" : 0,
					"midpoints" : [ 343.5, 404.0, 465.5, 404.0 ]
				}

			}
 ]
	}

}
