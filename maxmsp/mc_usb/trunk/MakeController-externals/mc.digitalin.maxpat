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
					"patching_rect" : [ 476.0, 195.0, 25.0, 25.0 ],
					"numinlets" : 1,
					"id" : "obj-12",
					"numoutlets" : 0,
					"comment" : "digital out 7"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"patching_rect" : [ 412.0, 195.0, 25.0, 25.0 ],
					"numinlets" : 1,
					"id" : "obj-11",
					"numoutlets" : 0,
					"comment" : "digital out 6"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"patching_rect" : [ 348.0, 195.0, 25.0, 25.0 ],
					"numinlets" : 1,
					"id" : "obj-10",
					"numoutlets" : 0,
					"comment" : "digital out 5"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"patching_rect" : [ 284.0, 195.0, 25.0, 25.0 ],
					"numinlets" : 1,
					"id" : "obj-9",
					"numoutlets" : 0,
					"comment" : "digital out 4"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"patching_rect" : [ 220.0, 195.0, 25.0, 25.0 ],
					"numinlets" : 1,
					"id" : "obj-8",
					"numoutlets" : 0,
					"comment" : "digital out 3"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"patching_rect" : [ 156.0, 195.0, 25.0, 25.0 ],
					"numinlets" : 1,
					"id" : "obj-7",
					"numoutlets" : 0,
					"comment" : "digital out 2"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"patching_rect" : [ 92.0, 195.0, 25.0, 25.0 ],
					"numinlets" : 1,
					"id" : "obj-6",
					"numoutlets" : 0,
					"comment" : "digital out 1"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "outlet",
					"patching_rect" : [ 27.0, 195.0, 25.0, 25.0 ],
					"numinlets" : 1,
					"id" : "obj-5",
					"numoutlets" : 0,
					"comment" : "digital out 0"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "inlet",
					"patching_rect" : [ 28.0, 87.0, 25.0, 25.0 ],
					"numinlets" : 0,
					"id" : "obj-2",
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"comment" : "OSC messages from the Make Controller"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "route /digitalin/0/value /digitalin/1/value /digitalin/2/value /digitalin/3/value /digitalin/4/value /digitalin/5/value /digitalin/6/value /digitalin/7/value",
					"linecount" : 2,
					"patching_rect" : [ 28.0, 126.0, 531.0, 34.0 ],
					"numinlets" : 1,
					"id" : "obj-1",
					"fontname" : "Arial",
					"numoutlets" : 9,
					"outlettype" : [ "", "", "", "", "", "", "", "", "" ],
					"fontsize" : 12.0
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-2", 0 ],
					"destination" : [ "obj-1", 0 ],
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
					"source" : [ "obj-1", 1 ],
					"destination" : [ "obj-6", 0 ],
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
					"source" : [ "obj-1", 3 ],
					"destination" : [ "obj-8", 0 ],
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
					"source" : [ "obj-1", 5 ],
					"destination" : [ "obj-10", 0 ],
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
					"source" : [ "obj-1", 7 ],
					"destination" : [ "obj-12", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
 ]
	}

}
