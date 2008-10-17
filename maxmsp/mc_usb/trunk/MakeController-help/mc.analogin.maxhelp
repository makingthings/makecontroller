{
	"patcher" : 	{
		"fileversion" : 1,
		"rect" : [ 168.0, 105.0, 910.0, 625.0 ],
		"bglocked" : 0,
		"defrect" : [ 168.0, 105.0, 910.0, 625.0 ],
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
					"maxclass" : "newobj",
					"text" : "s to-mc-usb",
					"fontsize" : 12.0,
					"id" : "obj-62",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 447.0, 216.0, 73.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Only one mc.usb object is required/allowed per patch - use the send (s) and receive (r) objects to pass info to and from it.",
					"linecount" : 3,
					"fontsize" : 12.0,
					"id" : "obj-57",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 601.0, 426.0, 246.0, 48.0 ],
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
					"patching_rect" : [ 656.0, 488.0, 71.0, 20.0 ],
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
					"patching_rect" : [ 656.0, 539.0, 87.0, 20.0 ],
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
					"patching_rect" : [ 656.0, 514.0, 51.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"bgcolor" : [ 0.807843, 0.807843, 0.807843, 1.0 ],
					"id" : "obj-61",
					"numinlets" : 1,
					"patching_rect" : [ 646.0, 476.0, 110.0, 97.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"varname" : "autohelp_see_title",
					"text" : "See Also:",
					"fontface" : 1,
					"fontsize" : 11.595187,
					"id" : "obj-1",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 676.0, 200.0, 100.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "umenu",
					"varname" : "autohelp_see_menu",
					"outlettype" : [ "int", "", "" ],
					"fontsize" : 11.595187,
					"items" : [ "(Objects:)", ",", "mc.usb" ],
					"id" : "obj-13",
					"numinlets" : 1,
					"fontname" : "Arial",
					"types" : [  ],
					"patching_rect" : [ 676.0, 220.0, 130.0, 20.0 ],
					"numoutlets" : 3
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "prepend help",
					"outlettype" : [ "" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-54",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 732.0, 251.0, 83.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pcontrol",
					"outlettype" : [ "" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-55",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 732.0, 276.0, 56.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "t b",
					"outlettype" : [ "bang" ],
					"hidden" : 1,
					"fontsize" : 9.0,
					"id" : "obj-15",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 727.0, 222.0, 19.0, 17.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "mc.analogin receives messages from the Make Controller Kit in its inlets, and sends analogin values out its outlets. \n\nOSC messages coming from the board via Ethernet go into the left inlet, and USB messages go in the right inlet.",
					"linecount" : 7,
					"fontsize" : 12.0,
					"id" : "obj-2",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 23.0, 84.0, 223.0, 103.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "mc.analogin",
					"fontsize" : 18.0,
					"frgb" : [ 0.082353, 0.219608, 0.035294, 1.0 ],
					"id" : "obj-3",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 12.0, 11.0, 106.0, 27.0 ],
					"numoutlets" : 0,
					"textcolor" : [ 0.082353, 0.219608, 0.035294, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Read analogin messages back from the Make Controller Kit.",
					"fontsize" : 12.0,
					"id" : "obj-4",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 13.0, 36.0, 386.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"bgcolor" : [ 0.905882, 0.905882, 0.905882, 1.0 ],
					"id" : "obj-5",
					"numinlets" : 1,
					"patching_rect" : [ 7.0, 6.0, 621.0, 53.0 ],
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
					"id" : "obj-6",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 321.0, 158.0, 260.0, 48.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "r from-mc-usb",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-7",
					"numinlets" : 0,
					"fontname" : "Arial",
					"patching_rect" : [ 448.0, 270.0, 85.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "gswitch2",
					"outlettype" : [ "", "" ],
					"int" : 1,
					"id" : "obj-8",
					"numinlets" : 2,
					"patching_rect" : [ 276.0, 153.0, 39.0, 32.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Once you have the value from your analogin, you can of course do whatever you like with it. Here, we're passing it into the pak object and then into the multislider object so we can get a visualization of the values coming in.",
					"linecount" : 6,
					"fontsize" : 12.0,
					"id" : "obj-11",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 541.0, 309.0, 221.0, 89.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "<- Says, \"read the value of \nall the analog inputs.\"",
					"linecount" : 2,
					"fontsize" : 12.0,
					"id" : "obj-12",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 404.0, 114.0, 175.0, 34.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "mc.analogin",
					"outlettype" : [ "list", "list", "list", "list", "list", "list", "list", "list" ],
					"fontsize" : 12.0,
					"id" : "obj-14",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 98.0, 307.0, 369.0, 20.0 ],
					"numoutlets" : 8
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "t b",
					"outlettype" : [ "bang" ],
					"hidden" : 1,
					"fontsize" : 12.0,
					"id" : "obj-16",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 855.0, 293.0, 22.0, 20.0 ],
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
					"id" : "obj-17",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 797.0, 229.0, 22.0, 20.0 ],
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
					"id" : "obj-18",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 826.0, 199.0, 22.0, 20.0 ],
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
					"id" : "obj-19",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 972.0, 353.0, 370.0, 32.0 ],
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
					"id" : "obj-20",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 889.0, 290.0, 369.0, 46.0 ],
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
					"id" : "obj-21",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 913.0, 245.0, 455.0, 32.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Make Controller Kit Discussion Forum",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-22",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 657.0, 166.0, 235.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Make Controller Kit OSC Reference",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-23",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 657.0, 143.0, 219.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "Make Controller Max/MSP Tutorial",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-24",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 657.0, 121.0, 223.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Check the MakingThings website for more info and reference material:",
					"linecount" : 2,
					"fontsize" : 12.0,
					"id" : "obj-25",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 658.0, 90.0, 221.0, 34.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "<- Click here to start reading.",
					"fontsize" : 12.0,
					"id" : "obj-26",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 323.0, 62.0, 392.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "<- Adjust this to the address \nof your board.",
					"linecount" : 2,
					"fontsize" : 12.0,
					"id" : "obj-27",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 276.0, 217.0, 167.0, 34.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "fpic",
					"id" : "obj-28",
					"numinlets" : 1,
					"patching_rect" : [ 638.0, 5.0, 223.0, 83.0 ],
					"pic" : "logo.png",
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "On/Off",
					"fontsize" : 12.0,
					"id" : "obj-29",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 251.0, 59.0, 48.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "7 (trimpot)",
					"fontsize" : 12.0,
					"id" : "obj-30",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 463.0, 344.0, 73.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "6",
					"fontsize" : 12.0,
					"id" : "obj-31",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 412.0, 344.0, 18.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "5",
					"fontsize" : 12.0,
					"id" : "obj-32",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 361.0, 344.0, 18.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "4",
					"fontsize" : 12.0,
					"id" : "obj-33",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 310.0, 344.0, 18.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "3",
					"fontsize" : 12.0,
					"id" : "obj-34",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 259.0, 344.0, 18.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "2",
					"fontsize" : 12.0,
					"id" : "obj-35",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 208.0, 344.0, 18.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "1",
					"fontsize" : 12.0,
					"id" : "obj-36",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 157.0, 344.0, 18.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "0",
					"fontsize" : 12.0,
					"id" : "obj-37",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 106.0, 344.0, 18.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"triscale" : 0.9,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-38",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 448.0, 368.0, 37.0, 20.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"triscale" : 0.9,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-39",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 398.0, 368.0, 37.0, 20.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"triscale" : 0.9,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-40",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 348.0, 368.0, 37.0, 20.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"triscale" : 0.9,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-41",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 298.0, 368.0, 37.0, 20.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"triscale" : 0.9,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-42",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 248.0, 368.0, 37.0, 20.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"triscale" : 0.9,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-43",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 198.0, 368.0, 37.0, 20.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"triscale" : 0.9,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-44",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 148.0, 368.0, 37.0, 20.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"outlettype" : [ "int", "bang" ],
					"triscale" : 0.9,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-45",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 98.0, 368.0, 37.0, 20.0 ],
					"numoutlets" : 2
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pak 0 0 0 0 0 0 0 0",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-46",
					"numinlets" : 8,
					"fontname" : "Arial",
					"patching_rect" : [ 98.0, 397.0, 369.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "multislider",
					"outlettype" : [ "", "" ],
					"settype" : 0,
					"candicane8" : [ 0.027451, 0.447059, 0.501961, 1.0 ],
					"setstyle" : 1,
					"candicane5" : [ 0.584314, 0.827451, 0.431373, 1.0 ],
					"candicane7" : [ 0.878431, 0.243137, 0.145098, 1.0 ],
					"candicane2" : [ 0.145098, 0.203922, 0.356863, 1.0 ],
					"slidercolor" : [ 0.419608, 0.419608, 0.419608, 1.0 ],
					"setminmax" : [ 0.0, 1023.0 ],
					"candycane" : 8,
					"id" : "obj-47",
					"numinlets" : 1,
					"candicane6" : [ 0.733333, 0.035294, 0.788235, 1.0 ],
					"size" : 8,
					"peakcolor" : [ 0.498039, 0.498039, 0.498039, 1.0 ],
					"patching_rect" : [ 98.0, 423.0, 481.0, 175.0 ],
					"numoutlets" : 2,
					"candicane4" : [ 0.439216, 0.619608, 0.070588, 1.0 ],
					"candicane3" : [ 0.290196, 0.411765, 0.713726, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Analog Inputs --",
					"fontsize" : 12.0,
					"id" : "obj-48",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 9.0, 344.0, 101.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "udpreceive 10000 MakeCtrl",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-49",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 98.0, 270.0, 165.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "/analogin/*/value",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"id" : "obj-50",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 296.0, 117.0, 102.0, 18.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"outlettype" : [ "int" ],
					"id" : "obj-51",
					"numinlets" : 1,
					"patching_rect" : [ 296.0, 61.0, 15.0, 15.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "metro 100",
					"outlettype" : [ "bang" ],
					"fontsize" : 12.0,
					"id" : "obj-52",
					"numinlets" : 2,
					"fontname" : "Arial",
					"patching_rect" : [ 296.0, 89.0, 75.0, 20.0 ],
					"numoutlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "udpsend 192.168.0.200 10000",
					"fontsize" : 12.0,
					"id" : "obj-53",
					"numinlets" : 1,
					"fontname" : "Arial",
					"patching_rect" : [ 98.0, 222.0, 174.0, 20.0 ],
					"numoutlets" : 0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"varname" : "autohelp_see_panel",
					"bgcolor" : [ 0.85, 0.85, 0.85, 0.75 ],
					"bordercolor" : [ 0.5, 0.5, 0.5, 0.75 ],
					"border" : 2,
					"background" : 1,
					"id" : "obj-56",
					"numinlets" : 1,
					"patching_rect" : [ 671.0, 196.0, 140.0, 50.0 ],
					"numoutlets" : 0
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-8", 1 ],
					"destination" : [ "obj-62", 0 ],
					"hidden" : 0,
					"midpoints" : [ 305.5, 207.0, 456.5, 207.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-7", 0 ],
					"destination" : [ "obj-14", 1 ],
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
					"source" : [ "obj-60", 0 ],
					"destination" : [ "obj-58", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-16", 0 ],
					"destination" : [ "obj-19", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-18", 0 ],
					"destination" : [ "obj-21", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-17", 0 ],
					"destination" : [ "obj-20", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-22", 0 ],
					"destination" : [ "obj-16", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-24", 0 ],
					"destination" : [ "obj-18", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-23", 0 ],
					"destination" : [ "obj-17", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-38", 0 ],
					"destination" : [ "obj-46", 7 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 7 ],
					"destination" : [ "obj-38", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-39", 0 ],
					"destination" : [ "obj-46", 6 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 6 ],
					"destination" : [ "obj-39", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-40", 0 ],
					"destination" : [ "obj-46", 5 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 5 ],
					"destination" : [ "obj-40", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-50", 0 ],
					"destination" : [ "obj-8", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-52", 0 ],
					"destination" : [ "obj-50", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-51", 0 ],
					"destination" : [ "obj-52", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-41", 0 ],
					"destination" : [ "obj-46", 4 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 4 ],
					"destination" : [ "obj-41", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-42", 0 ],
					"destination" : [ "obj-46", 3 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 3 ],
					"destination" : [ "obj-42", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-43", 0 ],
					"destination" : [ "obj-46", 2 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 2 ],
					"destination" : [ "obj-43", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-44", 0 ],
					"destination" : [ "obj-46", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 1 ],
					"destination" : [ "obj-44", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-46", 0 ],
					"destination" : [ "obj-47", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-45", 0 ],
					"destination" : [ "obj-46", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 0 ],
					"destination" : [ "obj-45", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-49", 0 ],
					"destination" : [ "obj-14", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-8", 0 ],
					"destination" : [ "obj-53", 0 ],
					"hidden" : 0,
					"midpoints" : [ 285.5, 208.0, 107.5, 208.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-13", 1 ],
					"destination" : [ "obj-54", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-54", 0 ],
					"destination" : [ "obj-55", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
 ]
	}

}
