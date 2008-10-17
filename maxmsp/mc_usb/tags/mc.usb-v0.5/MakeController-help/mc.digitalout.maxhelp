{
	"patcher" : 	{
		"fileversion" : 1,
		"rect" : [ 217.0, 130.0, 887.0, 333.0 ],
		"bglocked" : 0,
		"defrect" : [ 217.0, 130.0, 887.0, 333.0 ],
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
					"maxclass" : "comment",
					"text" : "Only one mc.usb object is required/allowed per patch - use the send (s) and receive (r) objects to pass info to and from it.",
					"linecount" : 4,
					"fontsize" : 12.0,
					"id" : "obj-57",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 658.0, 234.0, 192.0, 62.0 ],
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
					"patching_rect" : [ 550.0, 234.0, 71.0, 20.0 ],
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
					"patching_rect" : [ 550.0, 285.0, 87.0, 20.0 ],
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
					"patching_rect" : [ 550.0, 260.0, 51.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"bgcolor" : [ 0.807843, 0.807843, 0.807843, 1.0 ],
					"id" : "obj-61",
					"numinlets" : 1,
					"patching_rect" : [ 540.0, 222.0, 110.0, 97.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "mc.digitalout",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-1",
					"numinlets" : 8,
					"fontname" : "Arial",
					"patching_rect" : [ 268.0, 135.0, 320.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "7",
					"fontsize" : 12.0,
					"id" : "obj-2",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 576.0, 70.0, 26.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "6",
					"fontsize" : 12.0,
					"id" : "obj-3",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 533.0, 70.0, 26.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "5",
					"fontsize" : 12.0,
					"id" : "obj-4",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 490.0, 70.0, 26.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "4",
					"fontsize" : 12.0,
					"id" : "obj-5",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 447.0, 70.0, 26.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-6",
					"numinlets" : 1,
					"patching_rect" : [ 569.0, 89.0, 30.0, 30.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-7",
					"numinlets" : 1,
					"patching_rect" : [ 526.0, 89.0, 30.0, 30.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-8",
					"numinlets" : 1,
					"patching_rect" : [ 483.0, 89.0, 30.0, 30.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-9",
					"numinlets" : 1,
					"patching_rect" : [ 440.0, 89.0, 30.0, 30.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-10",
					"numinlets" : 1,
					"patching_rect" : [ 397.0, 89.0, 30.0, 30.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-11",
					"numinlets" : 1,
					"patching_rect" : [ 354.0, 89.0, 30.0, 30.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-12",
					"numinlets" : 1,
					"patching_rect" : [ 311.0, 89.0, 30.0, 30.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-13",
					"numinlets" : 1,
					"patching_rect" : [ 268.0, 89.0, 30.0, 30.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "3",
					"fontsize" : 12.0,
					"id" : "obj-14",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 404.0, 70.0, 26.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "2",
					"fontsize" : 12.0,
					"id" : "obj-15",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 361.0, 70.0, 26.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "1",
					"fontsize" : 12.0,
					"id" : "obj-16",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 318.0, 70.0, 26.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "0",
					"fontsize" : 12.0,
					"id" : "obj-17",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 275.0, 70.0, 26.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "mc.digitalout takes a 0 or a 1 and turns a digital output on the Make Application Board on or off accordingly. \n\nInlets 0-7 correspond to digital outs 0-7 on the board.",
					"linecount" : 7,
					"fontsize" : 12.0,
					"id" : "obj-19",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 33.0, 65.0, 202.0, 103.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "mc.digitalout",
					"fontsize" : 18.0,
					"frgb" : [ 0.082353, 0.219608, 0.035294, 1.0 ],
					"id" : "obj-20",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 13.0, 11.0, 126.0, 27.0 ],
					"numoutlets" : 0,
					"textcolor" : [ 0.082353, 0.219608, 0.035294, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Control the digital outputs on the Make Controller Kit.",
					"fontsize" : 12.0,
					"id" : "obj-21",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 13.0, 36.0, 385.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"bgcolor" : [ 0.905882, 0.905882, 0.905882, 1.0 ],
					"id" : "obj-22",
					"numinlets" : 1,
					"patching_rect" : [ 8.0, 6.0, 591.0, 53.0 ],
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
					"id" : "obj-23",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 292.0, 167.0, 260.0, 48.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "s to-mc-usb",
					"fontsize" : 12.0,
					"id" : "obj-24",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 422.0, 236.0, 73.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "gswitch2",
					"outlettype" : [ "", "" ],
					"int" : 1,
					"id" : "obj-25",
					"numinlets" : 2,
					"patching_rect" : [ 248.0, 168.0, 39.0, 32.0 ],
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
					"id" : "obj-26",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 820.0, 293.0, 22.0, 20.0 ],
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
					"id" : "obj-27",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 762.0, 229.0, 22.0, 20.0 ],
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
					"id" : "obj-28",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 791.0, 199.0, 22.0, 20.0 ],
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
					"id" : "obj-29",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 943.0, 327.0, 370.0, 32.0 ],
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
					"id" : "obj-30",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 889.0, 260.0, 369.0, 46.0 ],
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
					"id" : "obj-31",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 901.0, 212.0, 455.0, 32.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Make Controller Kit Discussion Forum",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-32",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 630.0, 176.0, 217.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Make Controller Kit OSC Reference",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-33",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 630.0, 152.0, 201.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Make Controller Max/MSP Tutorial",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-34",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 630.0, 129.0, 205.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Check the MakingThings website for more info and reference material:",
					"linecount" : 2,
					"fontsize" : 12.0,
					"id" : "obj-35",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 631.0, 95.0, 221.0, 34.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "<- Adjust this to the address of your board.",
					"linecount" : 2,
					"fontsize" : 12.0,
					"id" : "obj-36",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 266.0, 232.0, 147.0, 34.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "fpic",
					"id" : "obj-37",
					"numinlets" : 1,
					"patching_rect" : [ 611.0, 10.0, 228.0, 82.0 ],
					"pic" : "logo.png",
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "udpsend 192.168.0.200 10000",
					"fontsize" : 12.0,
					"id" : "obj-38",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 68.0, 234.0, 196.0, 20.0 ],
					"numoutlets" : 0
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-25", 1 ],
					"destination" : [ "obj-24", 0 ],
					"hidden" : 0,
					"midpoints" : [ 277.5, 222.0, 431.5, 222.0 ]
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
					"source" : [ "obj-60", 0 ],
					"destination" : [ "obj-58", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-26", 0 ],
					"destination" : [ "obj-29", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-28", 0 ],
					"destination" : [ "obj-31", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-27", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-32", 0 ],
					"destination" : [ "obj-26", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-34", 0 ],
					"destination" : [ "obj-28", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-33", 0 ],
					"destination" : [ "obj-27", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-6", 0 ],
					"destination" : [ "obj-1", 7 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-7", 0 ],
					"destination" : [ "obj-1", 6 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-8", 0 ],
					"destination" : [ "obj-1", 5 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-9", 0 ],
					"destination" : [ "obj-1", 4 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-10", 0 ],
					"destination" : [ "obj-1", 3 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-11", 0 ],
					"destination" : [ "obj-1", 2 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-12", 0 ],
					"destination" : [ "obj-1", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 0 ],
					"destination" : [ "obj-25", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-13", 0 ],
					"destination" : [ "obj-1", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-25", 0 ],
					"destination" : [ "obj-38", 0 ],
					"hidden" : 0,
					"midpoints" : [ 257.5, 222.0, 77.5, 222.0 ]
				}

			}
 ]
	}

}
