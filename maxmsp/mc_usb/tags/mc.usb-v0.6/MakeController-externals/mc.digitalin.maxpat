{
	"patcher" : 	{
		"fileversion" : 1,
		"rect" : [ 285.0, 135.0, 627.0, 275.0 ],
		"bglocked" : 0,
		"defrect" : [ 285.0, 135.0, 627.0, 275.0 ],
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
					"maxclass" : "outlet",
					"id" : "obj-12",
					"numinlets" : 1,
					"patching_rect" : [ 479.0, 195.0, 25.0, 25.0 ],
					"numoutlets" : 0,
					"comment" : "digital out 7"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"id" : "obj-11",
					"numinlets" : 1,
					"patching_rect" : [ 414.0, 195.0, 25.0, 25.0 ],
					"numoutlets" : 0,
					"comment" : "digital out 6"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"id" : "obj-10",
					"numinlets" : 1,
					"patching_rect" : [ 350.0, 195.0, 25.0, 25.0 ],
					"numoutlets" : 0,
					"comment" : "digital out 5"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"id" : "obj-9",
					"numinlets" : 1,
					"patching_rect" : [ 286.0, 195.0, 25.0, 25.0 ],
					"numoutlets" : 0,
					"comment" : "digital out 4"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"id" : "obj-8",
					"numinlets" : 1,
					"patching_rect" : [ 221.0, 195.0, 25.0, 25.0 ],
					"numoutlets" : 0,
					"comment" : "digital out 3"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"id" : "obj-7",
					"numinlets" : 1,
					"patching_rect" : [ 157.0, 195.0, 25.0, 25.0 ],
					"numoutlets" : 0,
					"comment" : "digital out 2"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"id" : "obj-6",
					"numinlets" : 1,
					"patching_rect" : [ 92.0, 195.0, 25.0, 25.0 ],
					"numoutlets" : 0,
					"comment" : "digital out 1"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"id" : "obj-5",
					"numinlets" : 1,
					"patching_rect" : [ 27.0, 195.0, 25.0, 25.0 ],
					"numoutlets" : 0,
					"comment" : "digital out 0"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "OpenSoundControl",
					"fontsize" : 12.0,
					"outlettype" : [ "", "", "OSCTimeTag" ],
					"id" : "obj-4",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 36.0, 65.0, 113.0, 20.0 ],
					"numoutlets" : 3
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "inlet",
					"outlettype" : [ "" ],
					"id" : "obj-3",
					"numinlets" : 0,
					"patching_rect" : [ 508.0, 56.0, 25.0, 25.0 ],
					"numoutlets" : 1,
					"comment" : "OSC messages from mc.usb"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "inlet",
					"outlettype" : [ "" ],
					"id" : "obj-2",
					"numinlets" : 0,
					"patching_rect" : [ 36.0, 23.0, 25.0, 25.0 ],
					"numoutlets" : 1,
					"comment" : "Ethernet OSC messages from udpreceive"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "osc-route /digitalin/0/value /digitalin/1/value /digitalin/2/value /digitalin/3/value /digitalin/4/value /digitalin/5/value /digitalin/6/value /digitalin/7/value",
					"linecount" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "list", "list", "list", "list", "list", "list", "list", "list", "list" ],
					"id" : "obj-1",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 28.0, 126.0, 534.0, 34.0 ],
					"numoutlets" : 9
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-1", 7 ],
					"destination" : [ "obj-12", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 6 ],
					"destination" : [ "obj-11", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 5 ],
					"destination" : [ "obj-10", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 4 ],
					"destination" : [ "obj-9", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 3 ],
					"destination" : [ "obj-8", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 2 ],
					"destination" : [ "obj-7", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 1 ],
					"destination" : [ "obj-6", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 0 ],
					"destination" : [ "obj-5", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-3", 0 ],
					"destination" : [ "obj-1", 0 ],
					"hidden" : 0,
					"midpoints" : [ 517.5, 100.0, 37.5, 100.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-4", 1 ],
					"destination" : [ "obj-1", 0 ],
					"hidden" : 0,
					"midpoints" : [ 92.5, 100.0, 37.5, 100.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-2", 0 ],
					"destination" : [ "obj-4", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
 ]
	}

}
