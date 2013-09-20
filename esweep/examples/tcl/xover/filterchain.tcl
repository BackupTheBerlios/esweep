namespace eval filterchain {
	# Filter definition: Type, Gain, Qp, Fp, Qz, Fz, canvasID
	variable filters
	array set filters {
		LastID -1
	}

	# group filters in one single symbol
	variable groups
        array set groups {
	}

	# Channel definition: Number XY-Anchor Chain
	variable channels
        array set channels {
	}

	variable config
	array set config {
		Canvas {}
		Canvas,Width 800
		Canvas,Height 600
		Channels {}
		Channel,cSize 100.0
		Groups {}
		Filters {}
		Selected {}
		Font "TkDefaultFont"
	}

	proc init {pathName editCmd} {
		variable config

		if {![winfo exists $pathName]} {
			return -code error "Pathname does not exist"
		}

		# create the canvas as slave of the frame
		set config(Canvas) [canvas $pathName.c -bg white]

		# pack it all together
		pack $config(Canvas) -fill both -expand yes -side left

		# binding for rescaling of the canvas
		bind $config(Canvas) <Configure> {::filterchain::redrawCanvas}

		# create bindings for selection
		bind $config(Canvas) <1> [list ::filterchain::select create %x %y]
		bind $config(Canvas) <Control-1> [list ::filterchain::select add %x %y]
		bind $config(Canvas) <3> [list ::filterchain::unselect all %x %y]

		set config(EditCommand) $editCmd
		# enable scanning on canvas
		bind $config(Canvas) <ButtonPress-2> [list ::filterchain::enableScanning $config(Canvas) %x %y]
		bind $config(Canvas) <ButtonRelease-2> [list ::filterchain::disableScanning $config(Canvas)]

		update

		return $config(Canvas)
	}

	proc redrawCanvas {} {
		variable config
		catch {after cancel $config(RefreshID)}
		set config(RefreshID) [after 100 ::filterchain::refreshCanvas]
	}

	proc refreshCanvas {} {
		variable config

		set c $config(Canvas)
		set newWidth [winfo width $c]
		set newHeight [winfo height $c]

		catch {set config(Channel,cSize) [expr {$newHeight / [llength $config(Channels)]}]}
		if {$config(Channel,cSize) > 100} {
			set config(Channel,cSize) 100
		} elseif {$config(Channel,cSize) < 10} {set config(Channel,cSize) 10}

		drawChain
	}

	proc enableScanning {w x y} {
		$w scan mark $x $y
		bind $w <Motion> [list $w scan dragto %x %y 3]
	}

	proc disableScanning {w} {
		bind $w <Motion> {}
	}

	proc findItem {x y} {
		variable config
		variable filters
		variable groups

		set c $config(Canvas)
		set xoff [$c canvasx 0]
		set yoff [$c canvasy 0]

		set d [expr {$config(Channel,cSize)/2}]
		set x1 [expr {$x+$xoff-$d}]
		set x2 [expr {$x+$xoff+$d}]
		set y1 [expr {$y+$yoff-$d}]
		set y2 [expr {$y+$yoff+$d}]

		set ID [$c find overlapping $x1 $y1 $x2 $y2]
		foreach id $ID {
			# search in groups
			foreach g $config(Groups) {
				# ignore the last element, to avoid
				# wrong selections
				if {[lsearch [lrange $groups($g,cID) 0 end-1] $id] > -1} {
					return [list Group $g]
				}
			}
			foreach f $config(Filters) {
				if {[lsearch [lrange $filters($f,cID) 0 end-1] $id] > -1} {
					return [list Filter $f]
				}
			}
		}
		return [list]
	}

	proc isSelected {ID} {
		variable config
		if {[lsearch -exact $config(Selected) $ID] > -1} {
			return 1
		} else {
			return 0
		}
	}

	proc select {mode x y} {
		variable config
		variable filters
		variable groups

		set c $config(Canvas)

		lassign [findItem $x $y] Type ID
		if {[isSelected $ID]} {
			if {$mode eq {add}} {
				unselect single $x $y
				return -code ok
			}
		}
		switch $Type {
			Group {
				# set/add this group to the selection
				if {$mode eq {add}} {
					lappend config(Selected) Group $ID
				} else {
					# unselect the old one
					unselect all $x $y
					set config(Selected) [list Group $ID]
				}
				# draw this group red
				foreach i $groups($ID,cID) {
					switch [$c type $i] {
						rectangle {
							$c itemconfigure $i -outline red
						}
						line {
							$c itemconfigure $i -fill red
						}
						text {
							$c itemconfigure $i -fill red
						}
					}
				}
				return -code ok
			}
			Filter {
				# set/add this group to the selection
				if {$mode eq {add}} {
					lappend config(Selected) Filter $ID
				} else {
					# unselect the old one
					unselect all $x $y
					set config(Selected) [list Filter $ID]
				}
				# draw this filter red
				foreach i $filters($ID,cID) {
					switch [$c type $i] {
						rectangle {
							$c itemconfigure $i -outline red
						}
						line {
							$c itemconfigure $i -fill red
						}
						text {
							$c itemconfigure $i -fill red
						}
					}
				}
				return -code ok
			}
			default {
				# nothing selected
			 	return -code ok
			}
		}
	}

	proc unselect {mode x y} {
		variable config
		variable filters
		variable groups

		set c $config(Canvas)

		if {$mode eq {all}} {
			set selection $config(Selected)
		} else {
			set selection [findItem $x $y]
		}
		foreach {Type ID} $selection {
			set idx [lsearch -exact $config(Selected) $ID]
			set config(Selected) [lreplace $config(Selected) [expr {$idx-1}] $idx]
			switch $Type {
				Group {
					# draw this group black
					foreach i $groups($ID,cID) {
						switch [$c type $i] {
							rectangle {
								$c itemconfigure $i -outline black
							}
							line {
								$c itemconfigure $i -fill black
							}
							text {
								$c itemconfigure $i -fill black
							}
						}
					}
				}
				Filter {
					# draw this filter black
					foreach i $filters($ID,cID) {
						switch [$c type $i] {
							rectangle {
								$c itemconfigure $i -outline black
							}
							line {
								$c itemconfigure $i -fill black
							}
							text {
								$c itemconfigure $i -fill black
							}
						}
					}
				}
				default {
					# nothing to unselect
				}
			}
		}
		return -code ok
	}

	proc groupSelected {} {
		variable config
		variable filters
		variable channels

		# is anything selected?
		if {[llength $config(Selected)] == 0} {
			# return silent
			return -code ok
		}
		# no groups can be grouped together,
		# so throw an error when the selection contains a group
		if {[lsearch $config(Selected) Group] > -1} {
			return -code error {Groups can not be grouped together.}
		}

		foreach {Type ID} $config(Selected) {
			lappend f $ID
		}

		# get the channel of the first ID
		# let createGroup check for other channels in the selection
		foreach ch $config(Channels) {
			if {[lsearch $channels($ch,Filters) [lsearch $f 0]] > -1} {break}
		}

		set name [enterNameDialog Group]
		createGroup $ch $name $f
		drawChannel $ch

		set config(Selected) [list]
	}

	proc expandSelected {} {
		variable config
		variable filters
		variable channels
		# is anything selected?
		if {[llength $config(Selected)] == 0} {
			# return silent
			return -code ok
		}
		# filters can't be expanded
		# so throw an error when the selections contains a filter
		if {[lsearch $config(Selected) Filter] > -1} {
			return -code error {Filters can not be expanded.}
		}

		foreach {Type ID} $config(Selected) {
			drawChannel [expandGroup $ID]
		}

		set config(Selected) [list]
	}

	proc editSelected {} {
		variable config
		variable filters
		variable channels

		# is anything selected?
		if {[llength $config(Selected)] == 0} {
			# return silent
			return -code ok
		}

		# only single filters can be edited
		if {[llength $config(Selected)] != 2} {
			return -code error {Edit only for single filters allowed}
		}
		if {[lsearch $config(Selected) Group] > -1} {
			return -code error {Groups can not be edited.}
		}
		set ID [lindex $config(Selected) 1]
		# find the filter channel
		foreach ch $config(Channels) {
			if {[lsearch $channels($ch,Filters) $ID] > -1} {break}
		}
		# get a copy of the filter array
		# because the edit dialog modifies the original array directly
		array set filter_copy [array get filters]

		if {[editDialog $ID] eq {}} {
			# edit cancelled, playback the copy
			array set filters [array get filter_copy]
			return -code ok {}
		} else {
			# filter was modified, draw the channel
			drawChannel $ch
			# call the external edit command with the channel name
			eval $config(EditCommand) $ch $ID
		}
	}

	proc deleteSelected {} {
		variable config
		variable channels

		# is anything selected?
		if {[llength $config(Selected)] == 0} {
			# return silent
			return -code ok
		}

		set mod_channels [list]
		foreach {Type ID} $config(Selected) {
			switch $Type {
				Filter {
          # find the affected channel
          foreach ch $config(Channels) {
            if {[lsearch $channels($ch,Filters) $ID] > -1} {lappend mod_channels $ch}
          }
					deleteFilter $ID
				}
				Group {
          # find the affected channel
          foreach ch $config(Channels) {
            if {[lsearch $channels($ch,Groups) $ID] > -1} {lappend mod_channels $ch}
          }
					deleteGroup $ID
				}
			}
		}

		drawChain
		# call the external edit command with the channel name
		eval [list $config(EditCommand) $mod_channels $ID]

		set config(Selected) [list]
	}

	proc editDialog {ID} {
		# give the window a unique name
		set w [toplevel .[clock seconds]]
		wm resizable $w 0 0
		wm title $w {Edit filter}

		grab $w

		bind $w <Escape> [list set $w {}]
		bind $w <Return> [list set $w 1]
		bind $w <KP_Enter> [list set $w 1]

		# retain a copy of the filter variables to write them back later
		array set tmp [array get ::filterchain::filters $ID,*]

		label $w.lt -text Type
		ttk::spinbox $w.type -values [list lowpass highpass bandpass bandstop allpass \
	       			notch shelve linkwitz integrator differentiator gain] \
				-state readonly \
				-wrap yes \
				-command [list ::filterchain::changeFilterTypeCmd \
				$w.type $w.qp $w.fp $w.qz $w.fz] \
				-textvariable ::filterchain::filters($ID,Type)

		label $w.lg -text Gain
		ttk::entry $w.gain -textvar ::filterchain::filters($ID,Gain) \
				-validate key \
				-validatecommand [list ::filterchain::valNumber %d %S %P]

		label $w.lfp -text Fp
		ttk::entry $w.fp -textvar ::filterchain::filters($ID,Fp) \
				-validate key \
				-validatecommand [list ::filterchain::valNumber %d %S %P {> 0}]

		label $w.lqp -text Qp
		ttk::entry $w.qp -textvar ::filterchain::filters($ID,Qp) \
				-validate key \
				-validatecommand [list ::filterchain::valNumber %d %S %P {> 0}]

		label $w.lfz -text Fz
		ttk::entry $w.fz -textvar ::filterchain::filters($ID,Fz) \
				-validate key \
				-validatecommand [list ::filterchain::valNumber %d %S %P {> 0}]

		label $w.lqz -text Qz
		ttk::entry $w.qz -textvar ::filterchain::filters($ID,Qz) \
				-validate key \
				-validatecommand [list ::filterchain::valNumber %d %S %P {> 0}]

		# execute the changeFilterTypeCmd once to enable/disable the widgets
		::filterchain::changeFilterTypeCmd $w.type $w.qp $w.fp $w.qz $w.fz
		button $w.ok     -text OK     -command [list set $w 1]
		button $w.cancel -text Cancel -command [list set $w {}]
		grid $w.lt	-	$w.type -sticky news
		grid $w.lg	-	$w.gain -sticky news
		grid $w.lfp	-	$w.fp -sticky news
		grid $w.lqp	-	$w.qp -sticky news
		grid $w.lfz	-	$w.fz -sticky news
		grid $w.lqz	-	$w.qz -sticky news
		grid $w.ok	x	$w.cancel
		focus $w
		vwait $w
		grab release $w

		destroy $w
		# check for empty values
		if {$::filterchain::filters($ID,Type) eq {}} {set ::filterchain::filters($ID,Type) $tmp($ID,Type)}
		if {$::filterchain::filters($ID,Gain) eq {}} {set ::filterchain::filters($ID,Gain) $tmp($ID,Gain)}
		if {$::filterchain::filters($ID,Fp) eq {}} {set ::filterchain::filters($ID,Fp) $tmp($ID,Fp)}
		if {$::filterchain::filters($ID,Qp) eq {}} {set ::filterchain::filters($ID,Qp) $tmp($ID,Qp)}
		if {$::filterchain::filters($ID,Fz) eq {}} {set ::filterchain::filters($ID,Fz) $tmp($ID,Fz)}
		if {$::filterchain::filters($ID,Qz) eq {}} {set ::filterchain::filters($ID,Qz) $tmp($ID,Qz)}

		set ::$w
	}

	proc valNumber {mode editStr newStr {constraint {}}} {
		if {$mode == 0} {
			# delete prevalidation
			# allow empty string
			if {$newStr eq {}} {
				return 1
			}
		}
		# do not allow an 'e' as input; otherwise it will probably recognized as a number
		if {$editStr eq {e}} {
			bell
			return 0
		}

		# is the string a number?
		if {[catch {expr {$newStr+1}}]} {
			bell
			return 0
		} else {
			# does it fulfill the constraint?
			if {$constraint ne {}} {
				# if the last element is a dot, remove it
				if {[string index $newStr end] eq {.}} {set newStr [string range $newStr 0 end-1]}
				if "$newStr $constraint" {
					return 1
				} else {
					bell
					return 0
				}
			}
		}
		return 1
	}

	# enable or disable windows dependent of selected filter type
	proc changeFilterTypeCmd {type Qp Fp Qz Fz} {
		switch [$type get] {
			lowpass {
				$Qp configure -state normal
				$Fp configure -state normal
				$Qz configure -state disabled
				$Fz configure -state disabled
			}
			highpass {
				$Qp configure -state normal
				$Fp configure -state normal
				$Qz configure -state disabled
				$Fz configure -state disabled
			}
			bandpass {
				$Qp configure -state normal
				$Fp configure -state normal
				$Qz configure -state disabled
				$Fz configure -state disabled
			}
			bandstop {
				$Qp configure -state normal
				$Fp configure -state normal
				$Qz configure -state disabled
				$Fz configure -state disabled
			}
			allpass {
				$Qp configure -state normal
				$Fp configure -state normal
				$Qz configure -state disabled
				$Fz configure -state disabled
			}
			notch {
				$Qp configure -state normal
				$Fp configure -state normal
				$Qz configure -state normal
				$Fz configure -state disabled
			}
			shelve {
				$Qp configure -state disabled
				$Fp configure -state normal
				$Qz configure -state disabled
				$Fz configure -state normal
			}
			linkwitz {
				$Qp configure -state normal
				$Fp configure -state normal
				$Qz configure -state normal
				$Fz configure -state normal
			}
			integrator {
				$Qp configure -state disabled
				$Fp configure -state normal
				$Qz configure -state disabled
				$Fz configure -state disabled
			}
			differentiator {
				$Qp configure -state disabled
				$Fp configure -state normal
				$Qz configure -state disabled
				$Fz configure -state disabled
			}
			gain {
				$Qp configure -state disabled
				$Fp configure -state disabled
				$Qz configure -state disabled
				$Fz configure -state disabled
			}
		}
	}

	proc enterNameDialog {whatToName} {
		# give the window a unique name
		set w [toplevel .[clock seconds]]
		wm resizable $w 0 0
		wm title $w {Name request}

		grab $w

		label  $w.l -text "Enter name of $whatToName"
		# and use the window name as the variables name
		# so it will not collide with global variables like "result"
		entry  $w.e -textvar $w -bg white
		bind $w.e <Return> {set done 1}
		button $w.ok     -text OK     -command {set done 1}
		button $w.c      -text Clear  -command "set $w {}"
		button $w.cancel -text Cancel -command "set $w {}; set done 1"
		grid $w.l  -    -        -sticky news
		grid $w.e  -    -        -sticky news
		grid $w.ok $w.c $w.cancel
		focus $w.e
		vwait done
		grab release $w

		destroy $w
		set ::$w
	}

	proc toggleSuppress {} {
		variable config
		variable filters
		variable groups
		variable channels

		# is anything selected?
		if {[llength $config(Selected)] == 0} {
			# return silent
			return -code ok
		}

		set mod_channels [list]
		foreach {Type ID} $config(Selected) {
			switch $Type {
				Filter {
					set filters($ID,Suppress) [expr {($filters($ID,Suppress)+1)%2}]
					# find the filter channel
					foreach ch $config(Channels) {
						if {[lsearch $channels($ch,Filters) $ID] > -1} {break}
					}
				}
				Group {
					set groups($ID,Suppress) [expr {($groups($ID,Suppress)+1)%2}]
					# mark all filters in this group
					foreach f $groups($ID,Filters) {
						set filters($f,Suppress) $groups($ID,Suppress)
					}

					# find the group channel
					foreach ch $config(Channels) {
						if {[lsearch $channels($ch,Groups) $ID] > -1} {break}
					}
				}
				default {
					continue
				}
			}
			# append the modified channel
			if {[lsearch $mod_channels $ch] < 0} {lappend mod_channels $ch}
		}

		# the chain was modified, redraw it and call the external edit command
		drawChannel $ch
		eval $config(EditCommand) $mod_channels $ID
	}

	proc drawFilter {ID anchor} {
		variable filters
		variable config

		set c $config(Canvas)

		# delete old ID
		foreach id $filters($ID,cID) {
			catch {$c delete $id}
		}
		set filters($ID,cID) [list]

		if {[lsearch $config(Selected) $ID] > -1} {set color red} else {set color black}
		lassign $anchor x0 y0
		# draw rectangle 50 x 50
		set dx $config(Channel,cSize)
		set dy [expr {$config(Channel,cSize)/2}]
		set filters($ID,cID) [concat $filters($ID,cID) [drawOutline $c $x0 $y0 $dx $dy $color]]
		set anchor [list [expr {$x0+2*$dx}] $y0]

		switch $filters($ID,Type) {
			{lowpass} {
				drawLowpass $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{highpass} {
				drawHighpass $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{bandpass} {
				drawBandpass $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{bandstop} {
				drawBandstop $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{allpass} {
				drawAllpass $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{notch} {
				drawNotch $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{shelve} {
				drawShelve $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{linkwitz} {
				drawLinkwitz $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{integrator} {
				drawIntegrator $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{differentiator} {
				drawDifferentiator $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			{gain} {
				drawGain $c $ID $x0 $y0 $dx $dy $config(Font) $color
			}
			default {
				# delete the last 2 IDs
				$c delete [lindex $filters($ID,cID) end]
				set filters($ID,cID) [lreplace $filters($ID,cID) end end]
				$c delete [lindex $filters($ID,cID) end]
				set filters($ID,cID) [lreplace $filters($ID,cID) end end]
				return -code error "\"$filters($ID,Type)\" no valid filter type"
			}
		}
		# cross out the filter if it is suppressed
		if {$filters($ID,Suppress)} {
			set y1 [expr {$y0-$dy}]
			set y2 [expr {$y0+$dy}]
			set x1 $x0
			set x2 [expr {$x0+$dx}]
			lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y2 -fill $color]
			lappend filters($ID,cID) [$c create line $x1 $y2 $x2 $y1 -fill $color]
		}

		# draw line-out
		# this mus be the last item, and will be ignored during selection
		set x1 [expr {$x0+$dx}]
		set x2 [expr {$x0+2*$dx}]
		lappend filters($ID,cID) [$c create line $x1 $y0 $x2 $y0 -fill $color]

		return $anchor
	}

	proc drawOutline {c x0 y0 dx dy color} {
		set x1 $x0
		set x2 [expr {$x0+$dx}]
		set y1 [expr {$y0-$dy}]
		set y2 [expr {$y0+$dy}]
		set id [$c create rectangle $x1 $y1 $x2 $y2 -outline $color]

		return $id
	}

	proc drawLowpass {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the lowpass symbol
		set x1 $x0
		set x2 [expr {$x0+$dx/2.0}]
		set x3 [expr {$x0+$dx}]
		set y1 [expr {$y0-4*$dy/5.0}]
		set y2 [expr {$y0+4*$dy/5.0}]
		lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y1 $x3 $y2 -fill $color]

		# draw text
		set fontheight [font metrics $font -displayof $c -linespace]
		set x1 [expr {$x0+$dx+2}]
		set y1 [expr {$y0}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Q: $filters($ID,Qp)" -fill $color]
		set y1 [expr {$y1+$fontheight}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "F: $filters($ID,Fp) Hz" -fill $color]
	}

	proc drawHighpass {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the lowpass symbol
		set x1 $x0
		set x2 [expr {$x0+$dx/2.0}]
		set x3 [expr {$x0+$dx}]
		set y1 [expr {$y0+4*$dy/5.0}]
		set y2 [expr {$y0-4*$dy/5.0}]
		lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y2 $x3 $y2 -fill $color]

		# draw text
		set fontheight [font metrics $font -displayof $c -linespace]
		set x1 [expr {$x0+$dx+2}]
		set y1 [expr {$y0}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Q: $filters($ID,Qp)" -fill $color]
		set y1 [expr {$y1+$fontheight}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "F: $filters($ID,Fp) Hz" -fill $color]
	}

	proc drawBandpass {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the bandpass symbol
		set x1 $x0
		set x2 [expr {$x0+$dx/3.0}]
		set x3 [expr {$x0+2*$dx/3.0}]
		set x4 [expr {$x0+$dx}]
		set y1 [expr {$y0-4*$dy/5.0}]
		set y2 [expr {$y0+4*$dy/5.0}]
		lappend filters($ID,cID) [$c create line $x1 $y2 $x2 $y1 $x3 $y1 $x4 $y2 -fill $color]

		# draw text
		set fontheight [font metrics $font -displayof $c -linespace]
		set x1 [expr {$x0+$dx+2}]
		set y1 [expr {$y0}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Q: $filters($ID,Qp)" -fill $color]
		set y1 [expr {$y1+$fontheight}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "F: $filters($ID,Fp) Hz" -fill $color]
	}

	proc drawBandstop {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the bandstop symbol
		set x1 $x0
		set x2 [expr {$x0+$dx/4.0}]
		set x3 [expr {$x0+2*$dx/4.0}]
		set x4 [expr {$x0+3*$dx/4.0}]
		set x5 [expr {$x0+$dx}]
		set y1 [expr {$y0-4*$dy/5.0}]
		set y2 [expr {$y0+4*$dy/5.0}]
		lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y1 $x3 $y2 $x4 $y1 $x5 $y1 -fill $color]

		# draw text
		set fontheight [font metrics $font -displayof $c -linespace]
		set x1 [expr {$x0+$dx+2}]
		set y1 [expr {$y0}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Q: $filters($ID,Qp)" -fill $color]
		set y1 [expr {$y1+$fontheight}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "F: $filters($ID,Fp) Hz" -fill $color]
	}

	proc drawAllpass {c ID x0 y0 dx dy font color} {
		variable filters

		# write "T" to indicate the group delay
		set x1 [expr {$x0+$dx/2.0}]
		set fontheight [font metrics $font -displayof $c -linespace]
		lappend filters($ID,cID) \
			[$c create text $x1 $y0 -font $font -anchor c -text "Tg" -fill $color]


		# draw text
		set x1 [expr {$x0+$dx+2}]
		set y1 [expr {$y0}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Q: $filters($ID,Qp)" -fill $color]
		set y1 [expr {$y1+$fontheight}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "F: $filters($ID,Fp) Hz" -fill $color]
	}

	proc drawNotch {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the notch symbol
		set x1 $x0
		set x2 [expr {$x0+$dx/5.0}]
		set x3 [expr {$x0+2*$dx/5.0}]
		set x4 [expr {$x0+3*$dx/5.0}]
		set x5 [expr {$x0+4*$dx/5.0}]
		set x6 [expr {$x0+$dx}]
		set y1 [expr {$y0-4*$dy/5.0}]
		set y2 $y0
		lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y1 $x3 $y2 $x4 $y2 $x5 $y1 $x6 $y1 -fill $color]

		# draw text
		set fontheight [font metrics $font -displayof $c -linespace]
		set x1 [expr {$x0+$dx+2}]
		set y1 [expr {$y0-2*$fontheight-2}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Qz: $filters($ID,Qz)" -fill $color]
		set y1 [expr {$y1+$fontheight}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Fz: $filters($ID,Fz) Hz" -fill $color]
		set y1 [expr {$y1+$fontheight+2}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Qp: $filters($ID,Qp)" -fill $color]
		set y1 [expr {$y1+$fontheight}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Fp: $filters($ID,Fp) Hz" -fill $color]
	}

	proc drawShelve {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the notch symbol
		set x1 $x0
		set x2 [expr {$x0+$dx/3.0}]
		set x3 [expr {$x0+2*$dx/3.0}]
		set x4 [expr {$x0+$dx}]
		if {$filters($ID,Fz) >= $filters($ID,Fp)} {
			set y1 [expr {$y0-4*$dy/5.0}]
			set y2 $y0
		} else {
			set y1 $y0
			set y2 [expr {$y0-4*$dy/5.0}]
		}
		lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y1 $x3 $y2 $x4 $y2 -fill $color]

		# draw text
		set fontheight [font metrics $font -displayof $c -linespace]
		set x1 [expr {$x0+$dx+2}]
		set y1 [expr {$y0-$fontheight-2}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Fz: $filters($ID,Fz) Hz" -fill $color]
		set y1 [expr {$y1+2*$fontheight+2}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Fp: $filters($ID,Fp) Hz" -fill $color]
	}

	proc drawLinkwitz {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the notch symbol
		set x1 $x0
		set x2 [expr {$x0+$dx/5.0}]
		set x3 [expr {$x0+2*$dx/5.0}]
		set x4 [expr {$x0+3*$dx/5.0}]
		set x5 [expr {$x0+4*$dx/5.0}]
		set x6 [expr {$x0+$dx}]
		set y1 [expr {$y0-4*$dy/5.0}]
		set y2 [expr {$y0+4*$dy/5.0}]
		set y3 $y0
		lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y1 $x3 $y2 $x4 $y2 $x5 $y3 $x6 $y3 -fill $color]

		# draw text
		set fontheight [font metrics $font -displayof $c -linespace]
		set x1 [expr {$x0+$dx+2}]
		set y1 [expr {$y0-2*$fontheight-2}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Qz: $filters($ID,Qz)" -fill $color]
		set y1 [expr {$y1+$fontheight}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Fz: $filters($ID,Fz) Hz" -fill $color]
		set y1 [expr {$y1+$fontheight+2}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Qp: $filters($ID,Qp)" -fill $color]
		set y1 [expr {$y1+$fontheight}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y1 -font $font -anchor nw -text "Fp: $filters($ID,Fp) Hz" -fill $color]
	}

	proc drawIntegrator {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the notch symbol
		set x1 $x0
		set x2 [expr {$x0+$dx}]
		set y1 [expr {$y0-4*$dy/5.0}]
		set y2 [expr {$y0+4*$dy/5.0}]
		lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y2 -fill $color]

		# no text
	}

	proc drawDifferentiator {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the notch symbol
		set x1 $x0
		set x2 [expr {$x0+$dx}]
		set y1 [expr {$y0+4*$dy/5.0}]
		set y2 [expr {$y0-4*$dy/5.0}]
		lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y2 -fill $color]

		# no text
	}

	proc drawGain {c ID x0 y0 dx dy font color} {
		variable filters

		# draw the bandpass symbol
		set x1 $x0
		set x2 [expr {$x0+$dx}]
		set y1 [expr {$y0-4*$dy/5.0}]
		lappend filters($ID,cID) [$c create line $x1 $y1 $x2 $y1 -fill $color]

		# draw the gain
		set x1 [expr {$x0+$dx/2.0}]
		lappend filters($ID,cID) \
			[$c create text $x1 $y0 -font $font -anchor c -text [format {%.2f} $filters($ID,Gain)] -fill $color]
	}

	proc drawGroup {Name anchor} {
		variable groups
		variable config

		set c $config(Canvas)
		foreach id $groups($Name,cID) {
			catch {$c delete $id}
		}
		# if this group is selected, draw it red
		if {[lsearch $config(Selected) $Name] > -1} {set color red} else {set color black}
		lassign $anchor x0 y0
		# draw rectangle 50 x 50
		set dx $config(Channel,cSize)
		set dy [expr {$config(Channel,cSize)/2}]
		set groups($Name,cID) [concat $groups($Name,cID) [drawOutline $c $x0 $y0 $dx $dy $color]]
		lappend groups($Name,cID) \
			[$c create text [expr {$x0+$dx/2.0}] $y0 -font $config(Font) -anchor c -text $Name -fill $color]

		# draw two little rectangles
		set x1 $x0
		set x2 [expr {$x0+$dx/5.0}]
		set x3 [expr {$x0+2*$dx/5.0}]
		set x4 [expr {$x0+3*$dx/5.0}]
		set x5 [expr {$x0+4*$dx/5.0}]
		set x6 [expr {$x0+$dx}]
		set y1 [expr {$y0-4*$dy/5.0}]
		set y2 [expr {$y1+$dy/5.0}]
		set y3 [expr {$y1+2*$dy/5.0}]
		lappend groups($Name,cID) [$c create line $x1 $y2 $x2 $y2 -fill $color]
		lappend groups($Name,cID) [$c create rectangle $x2 $y1 $x3 $y3 -outline $color]
		lappend groups($Name,cID) [$c create line $x3 $y2 $x4 $y2 -fill $color]
		lappend groups($Name,cID) [$c create rectangle $x4 $y1 $x5 $y3 -outline $color]
		lappend groups($Name,cID) [$c create line $x5 $y2 $x6 $y2 -fill $color]

		# cross out the filter if it is suppressed
		if {$groups($Name,Suppress)} {
			set y1 [expr {$y0-$dy}]
			set y2 [expr {$y0+$dy}]
			set x1 $x0
			set x2 [expr {$x0+$dx}]
			lappend groups($Name,cID) [$c create line $x1 $y1 $x2 $y2 -fill $color]
			lappend groups($Name,cID) [$c create line $x1 $y2 $x2 $y1 -fill $color]
		}

		# draw line-out
		# this mus be the last item, and will be ignored during selection
		set x1 [expr {$x0+$dx}]
		set x2 [expr {$x0+2*$dx}]
		lappend groups($Name,cID) [$c create line $x1 $y0 $x2 $y0 -fill $color]
		return [list $x2 $y0]
	}

	proc drawChannel {Name} {
		variable config
		variable filters
		variable channels

		set c $config(Canvas)

		set x0 $channels($Name,X0)
		set y0 $channels($Name,Y0)
		set dx $config(Channel,cSize)
		set dy [expr {$config(Channel,cSize)/2}]

		# delete the old channel
		foreach id $channels($Name,cID) {
			catch {$c delete $id}
		}
		set channels($Name,cID) [list]
		# draw channel name
		set textwidth [font measure $config(Font) -displayof $c $Name]
		lappend channels($Name,cID) \
			[$c create text $x0 $y0 -font $config(Font) -anchor w -text $Name]

		set x1 [expr {$x0+$textwidth+2}]
		if {$x1 > $x0 + $dx} {
			set x2 [expr {$x1+10}]
		} else {
			set x2 [expr {$x0+$dx}]
		}
		lappend channels($Name,cID) [$c create line $x1 $y0 $x2 $y0]

		# now draw the filters of this channel
		set anchor [list $x2 $y0]
		set groups [list]
		foreach f $channels($Name,Filters) {
			# if the filter is not grouped, draw it
			if {$filters($f,Group) ne {}} {
				foreach id $filters($f,cID) {
					catch {$c delete $id}
				}
				# remember group
				if {[lsearch $groups $filters($f,Group)] < 0} {
					lappend groups $filters($f,Group)
					# draw it
					set anchor [drawGroup $filters($f,Group) $anchor]
				}
			} else {
				set anchor [drawFilter $f $anchor]
			}
		}

		# done
		return -code ok
	}

	# this function only needs to be called
	proc drawChain {} {
		variable config
		variable groups
		variable filters
		variable channels

		foreach ch $config(Channels) {
			drawChannel $ch
		}
		return -code ok
	}

	proc addChannel {Name Map} {
		variable config
		variable channels

		# does it already exist?
		if {[info exists channels($Name,Map)]} {return -code error "Channel $Name already exists!"}
		# is it a valid channel number?
		set m $Map
		if {[catch {incr m}]} {return -code error "Channel number $Map not an integer"}
		if {$Map <= 0} {return -code error "Channel number $Map <= 0"}
		# is the channel number mapped?
		foreach ch $config(Channels) {
			if {$channels($ch,Map) == $Map} {return -code error "Channel number $Map already mapped!"}
		}
		# create standard channel description

		set channels($Name,Map) $Map
		set channels($Name,X0) 2
		# use factor 1.1 to get some margin between the channels
		set channels($Name,Y0) [expr {1.1*(0.5+[llength $config(Channels)])*$config(Channel,cSize)}]
		set channels($Name,Filters) [list]
		set channels($Name,Groups) [list]
		set channels($Name,cID) [list]

		lappend config(Channels) $Name

		return -code ok
	}

	proc deleteChannel {Name} {
		variable config
		variable channels
		variable filters

		if {$Name eq {#all}} {
			set names $config(Channels)
		}

		foreach ch $names {
			# does the channel exist?
			if {![info exists channels($ch,Map)]} {
				return -code error "Channel $ch does not exist!"
			}
			# delete channel from canvas
			set c $config(Canvas)
			foreach id $channels($ch,cID) {
				catch {$c delete $id}
			}

			# delete all groups
			foreach g $channels($ch,Groups) {
				deleteGroup $g
			}

			# delete the remaining filters
			foreach f $channels($ch,Filters) {
				deleteFilter $f
			}

			# remove it from the config
			set idx [lsearch -exact $config(Channels) $ch]
			set config(Channels) [lreplace $config(Channels) $idx $idx]

			array unset channels $ch,*
		}

		return -code ok
	}

	proc addFilter {Channel Type Gain Qp Fp Qz Fz} {
		variable config
		variable channels
		variable filters

		# does the channel exist?
		if {![info exists channels($Channel,Map)]} {return -code error "Channel $Channel does not exist!"}

		# get the ID
		set ID [incr filters(LastID)]
		set filters($ID,Type) $Type
		set filters($ID,Gain) $Gain
		set filters($ID,Qp) $Qp
		set filters($ID,Fp) $Fp
		set filters($ID,Qz) $Qz
		set filters($ID,Fz) $Fz
		set filters($ID,cID) {}
		set filters($ID,Group) {}
		set filters($ID,Suppress) 0

		lappend channels($Channel,Filters) $ID
		lappend config(Filters) $ID

		# do not draw the channel now. You can add more filters, and then do one single drawing command
		return -code ok $ID
	}

	proc deleteFilter {ID} {
		variable config
		variable channels
		variable filters
		variable groups

		# does the filter exist?
		if {![info exists filters($ID,Suppress)]} {return -code error "Filter with ID $ID does not exist!"}

		# remove this filter from the canvas
		set c $config(Canvas)
		foreach id $filters($ID,cID) {
			catch {$c delete $id}
		}

		# find the filter channel
		foreach ch $config(Channels) {
			if {[lsearch $channels($ch,Filters) $ID] > -1} {break}
		}

		# remove this filter from the channel
		set idx [lsearch -exact $channels($ch,Filters) $ID]
		set channels($ch,Filters) [lreplace $channels($ch,Filters) $idx $idx]

		# remove from any group when necessary
		if {[set g $filters($ID,Group)] ne {}} {
			set idx [lsearch -exact $groups($g,Filters) $ID]
			set groups($g,Filters) [lreplace $groups($g,Filters) $idx $idx]
		}

		# remove this filter from the configuration
		set idx [lsearch -exact $config(Filters) $ID]
		set config(Filters) [lreplace $config(Filters) $idx $idx]

		# delete the filter
		array unset filters $ID,*
		return -code ok
	}

	proc createGroup {Channel Name Filters} {
		variable config
		variable channels
		variable filters
		variable groups

		# does the channel exist?
		if {![info exists channels($Channel,Map)]} {
			return -code error "Channel $Channel does not exist!"
		}

		# does this group already exist?
		if {[info exists groups($Name,Suppress)]} {
			return -code error "Group $Name already exists!"
		}

		if {[llength $Filters] < 1} {
			return -code error "No filters supplied!"
		}

		# first check the filters
		foreach f $Filters {
			# does it exist?
			if {![info exists filters($f,Suppress)]} {
				return -code error "Filter $f does not exist!"
			}
			# does it belong to this channel?
			if {[lsearch $channels($Channel,Filters) $f] < 0} {
				return -code error "Filter $f does not belong to channel $Channel!"
			}
		}

		# create group
		set groups($Name,cID) [list]
		set groups($Name,Filters) [list]
		set groups($Name,Suppress) 0

		lappend config(Groups) $Name
		lappend channels($Channel,Groups) $Name

		# now add the filters to the group
		foreach f $Filters {
			lappend groups($Name,Filters) $f
			set filters($f,Group) $Name
		}
		return -code ok
	}

	# delete the group, but retain the filters
	proc expandGroup {Name} {
		variable config
		variable groups
		variable filters
		variable channels
		# does the group exist?
		if {![info exists groups($Name,Suppress)]} {
			return -code error "Group $Name does not exist!"
		}

		# remove this group from the canvas
		set c $config(Canvas)
		foreach id $groups($Name,cID) {
			catch {$c delete $id}
		}

		foreach f $groups($Name,Filters) {
			set filters($f,Group) {}
		}
		array unset groups $Name,*
		set idx [lsearch -exact $config(Groups) $Name]
		set config(Groups) [lreplace $config(Groups) $idx $idx]

		# find the group channel
		foreach ch $config(Channels) {
			if {[lsearch $channels($ch,Filters) $Name] > -1} {break}
		}
		set idx [lsearch -exact $channels($ch,Groups) $Name]
		set channels($ch,Groups) [lreplace $channels($ch,Groups) $idx $idx]
		return -code ok $ch
	}

	# delete the group including the filters
	proc deleteGroup {Name} {
		variable config
		variable groups
		variable filters
		variable channels
		# does the group exist?
		if {![info exists groups($Name,Suppress)]} {
			return -code error "Group $Name does not exist!"
		}

		# remove this group from the canvas
		set c $config(Canvas)
		foreach id $groups($Name,cID) {
			catch {$c delete $id}
		}

		foreach f $groups($Name,Filters) {
			deleteFilter $f
		}
		array unset groups $Name,*
		set idx [lsearch -exact $config(Groups) $Name]
		set config(Groups) [lreplace $config(Groups) $idx $idx]
		# find the group channel
		foreach ch $config(Channels) {
			if {[lsearch $channels($ch,Filters) $Name] > -1} {break}
		}
		set idx [lsearch -exact $channels($ch,Groups) $Name]
		set channels($ch,Groups) [lreplace $channels($ch,Groups) $idx $idx]
		return -code ok
	}

	proc getChannels {} {
		variable config
		return $config(Channels)
	}

	proc getMap {channel} {
		variable config
		variable channels
		# does the channel exist?
		if {![info exists channels($channel,Map)]} {
			return -code error "Channel $channel does not exist!"
		}
		return $channels($channel,Map)
	}

	proc getFilters {Channel} {
		variable filters
		variable channels

		# does the channel exist?
		if {![info exists channels($Channel,Map)]} {
			return -code error "Channel $Channel does not exist!"
		}

		set coeffs [list]
		foreach f $channels($Channel,Filters) {
			if {$filters($f,Suppress)} {continue}
			lappend coeffs [list \
				$filters($f,Type)\
				$filters($f,Gain)\
				$filters($f,Qp)\
				$filters($f,Fp)\
				$filters($f,Qz)\
				$filters($f,Fz)]
		}
		return $coeffs
	}

	proc saveChain {filename} {
		variable config
		variable filters
		variable channels
		variable groups

		if {[set fp [open $filename w]] eq {}} {
			return -code error "Unable to open file $filename"
		}
		puts $fp "channels {"
		foreach ch $config(Channels) {
			puts $fp "\t$ch,Groups {$channels($ch,Groups)}"
			puts $fp "\t$ch,Filters {$channels($ch,Filters)}"
			puts $fp "\t$ch,Map {$channels($ch,Map)}"
		}
		puts $fp "}\n"

		puts $fp "filters {"
		foreach f $config(Filters) {
			foreach name [array names filters $f,* ] {
				if {$name eq "$f,X0" || $name eq "$f,Y0" || $name eq "$f,cID"} continue
				puts $fp "\t$name {$filters($name)}"
			}
		}
		puts $fp "}\n"

		puts $fp "groups {"
		foreach g $config(Groups) {
			foreach name [array names groups $g,* ] {
				if {$name eq "$g,cID"} continue
				puts $fp "\t$name {$groups($name)}"
			}
		}
		puts $fp "}\n"

		close $fp
	}

	proc loadChain {filename} {
		if {[set fp [open $filename r]] eq {}} {
			return -code error "Unable to open file $filename"
		}
		# read file at once
		set data [read $fp]
		close $fp

		# fill the local arrays
		foreach {ary values} $data {
			array set $ary $values
		}

		# the array "channels", "filters" and "groups" must exist
		if {![array exists channels]} {
			return -code error {Syntax error: no channel definition}
		}
		if {![array exists filters]} {
			return -code error {Syntax error: no filter definition}
		}
		if {![array exists groups]} {
			return -code error {Syntax error: no group definition}
		}

		# delete the old channels
		deleteChannel {#all}

		# create new channels
		foreach ch [array names channels *,Map] {
			set name [lrange [split $ch ,] 0 end-1]
			addChannel $name $channels($name,Map)
			foreach f $channels($name,Filters) {
				addFilter $name $filters($f,Type) \
					$filters($f,Gain) \
					$filters($f,Qp) $filters($f,Fp) \
					$filters($f,Qz) $filters($f,Fz)
				#setSuppress Filter $f $filters($f,Suppress)
			}

		}

		# add the groups
		foreach ch [array names channels *,Map] {
			set name [lrange [split $ch ,] 0 end-1]
			foreach g $channels($name,Groups) {
				createGroup $name $g $groups($g,Filters)
			}
		}
		drawChain
	}
}

