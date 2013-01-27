package provide TkEsweepXYPlot 0.1

# This package assumes that the esweep library is loaded into the current interpreter

##########################
# Principle of operation #
##########################

namespace eval TkEsweepXYPlot {
	variable def_config
	array set def_config {
		Margin,Left 10
		Margin,Right 10
		Margin,Bottom 15
		Margin,Top 15

		Legend,Position "bottom"
		Legend,Align "center"
		Legend,Symbol,Length 10
		Legend,Border,Width 2 

		X,Log "no"
		Y1,Log "no"
		Y2,Log "no"

		X,State "on"
		Y1,State "on"
		Y2,State "off"

		X,Autoscale "off"
		Y1,Autoscale "off"
		Y2,Autoscale "off"

		X,Min 0
		X,Max 1
		X,Range {}
		X,Bounds {}

		Y1,Min 0
		Y1,Max 1
		Y1,Range {}
		Y1,Bounds {}

		Y2,Min 0
		Y2,Max 1
		Y2,Range {}
		Y2,Bounds {}
		
		X,Step 3
		X,StepDash ""
		X,MStep 10
		X,MStepDash .
		X,Precision 2

		Y1,Step 5
		Y1,StepDash ""
		Y1,MStep 1
		Y1,MStepDash .
		Y1,Precision 2

		Y2,Step 10
		Y2,StepDash ""
		Y2,MStep 1
		Y2,MStepDash .
		Y2,Precision 2

		X,Label "x"
		Y1,Label "y1"
		Y2,Label "y2"

		Title ""

		Y1,Style "lines"
		Y2,Style "lines"

		Cursors,1 {on}
		Cursors,2 {on}
		Cursors,1,X 100
		Cursors,2,X 10e3
		Cursors,1,Color {darkblue}
		Cursors,2,Color {darkgreen}
		Cursors,1,Width 2
		Cursors,2,Width 2

		Markers {on}

		BorderDash ""

		Font "TkDefaultFont"

		MouseHoverActive "yes"
		MouseHoverDeleteDelay 1000

		Dirty 0
	}

	variable events
	array set events {}

	variable plots
	array set plots {}

	variable cData
	array set cData {}
}

proc ::TkEsweepXYPlot::init {pathName plotName {title ""}} {
	variable cData
	variable plots
	variable def_config

	if {![winfo exists $pathName]} {
		return -code error "Pathname does not exist"
	}
	# create the canvas as slave of the frame
	set c [canvas $pathName.c -bg white]
	set cData($plotName,pathName) $c
	set cData($plotName,prevWidth) [winfo width $c]
	set cData($plotName,prevHeight) [winfo height $c]
	$c configure -bg white
	set cData($plotName,bgColor) white
	# pack it all together
	pack $c -fill both -expand yes -side left

	lappend plots(Names) $plotName
	set plots($plotName,Traces) [list]

	foreach item [array names def_config] {
		set plots($plotName,Config,$item) $def_config($item)
	}
	set plots($plotName,Config,Title) $title
	# draw coordinate system
	set plots($plotName,Config,Dirty) 1

	bind $c <Motion> [list ::TkEsweepXYPlot::mouseHoverReadout $plotName %x %y]

	bind $c <ButtonPress-1> [list  ::TkEsweepXYPlot::setCursor $plotName 1 %x %y]
	bind $c <ButtonPress-3> [list  ::TkEsweepXYPlot::setCursor $plotName 2 %x %y]

	bind $c <Configure> [list ::TkEsweepXYPlot::redrawCanvas $plotName]


	return $plotName
}

proc ::TkEsweepXYPlot::redrawCanvas {plotName} {
	variable cData 
	catch {after cancel $cData($plotName,refreshID)}
	set cData($plotName,refreshID) [after 100 ::TkEsweepXYPlot::refreshCanvas $plotName]
}

proc ::TkEsweepXYPlot::refreshCanvas {plotName} {
	variable cData 
	variable plots

	set c $cData($plotName,pathName)
	set newWidth [winfo width $c]
	set newHeight [winfo height $c]

	set wscale [expr {1.0*$newWidth/$cData($plotName,prevWidth)}]
	set hscale [expr {1.0*$newHeight/$cData($plotName,prevHeight)}]
	set cData($plotName,prevWidth) $newWidth
	set cData($plotName,prevHeight) $newHeight
	# make a full redraw if the change in the window size is too big
	if {$wscale > 2 || $wscale < 0.5 || $hscale > 2 || $hscale < 0.5} {
		set plots($plotName,Config,Dirty) 1
		# ignore errors, because this plot may nor exist yet
		# but the canvas does, and gets configured for a redraw
		catch {::TkEsweepXYPlot::plot $plotName}
	} else {
		# scale all elements
		$c scale all 0 0 $wscale $hscale
		# draw the cursors

	}
}

# add a trace to the plot
# obj is an esweep object
# args contains options for this trace

proc ::TkEsweepXYPlot::addTrace {plotName traceID traceName obj} {
	variable plots

	if {[exists $plotName $traceID]} {
		return -code error "Trace $traceID already exists on this plot!"
	}
	# test if it is an esweep object and if the size is greater zero
	if {[catch {set size [esweep::size -obj $obj]}]} {return -1}
	if {$size <= 0} {return -1}
	if {$traceName eq {}} {return -1}

	lappend plots($plotName,Traces) $traceID
	# set defaults
	# Scale: defines which part of the object is displayed (Y1=abs/real, Y2=arg/imag, BOTH: on both scales)
	set plots($plotName,Traces,$traceID,Scale) Y1
	# State: is it displayed or not
	set plots($plotName,Traces,$traceID,State) on
	# Color: which color to use
	set plots($plotName,Traces,$traceID,Color) red
	# Width: pen width (0 is default pen)
	set plots($plotName,Traces,$traceID,Width) 0

	set plots($plotName,Traces,$traceID,Name) $traceName
	set plots($plotName,Traces,$traceID,Obj) $obj
	set plots($plotName,Traces,$traceID,Markers) [list]
	set plots($plotName,Traces,$traceID,Dirty) 1
	set plots($plotName,Traces,$traceID,ID1) {}
	set plots($plotName,Traces,$traceID,ID2) {}
	set plots($plotName,Config,Dirty) 1

	return $traceID
}

proc ::TkEsweepXYPlot::getTrace {plotName traceID} {
	variable plots

	if {[info exists plots($plotName,Traces)]==0} return
	if {[info exists plots($plotName,Traces,$traceID,Scale)]==0} {
		return -code error "Trace or plot does not exist!"
	}

	return $plots($plotName,Traces,$traceID,Obj)
}
	
proc ::TkEsweepXYPlot::removeTrace {plotName traceID} {
	variable plots
	variable cData
	
	if {[info exists plots($plotName,Traces)]==0} return

	if {$traceID eq {all}} {
		set traceID $plots($plotName,Traces)
	}

	set c $cData($plotName,pathName)

	foreach id $traceID {
		if {[info exists plots($plotName,Traces,$id,Scale)]==0} {
			return -code error "Trace or plot does not exist!"
		}
		catch {$c delete $plots($plotName,Traces,$id,ID1)}
		catch {$c delete $plots($plotName,Traces,$id,ID2)}

		array unset plots $plotName,Traces,$id,*
		set i [lsearch $plots($plotName,Traces) $id]
		set plots($plotName,Traces) [lreplace $plots($plotName,Traces) $i $i]
	}

	set plots($plotName,Config,Dirty) 1

	return true
}

proc ::TkEsweepXYPlot::configTrace {plotName traceID args} {
	variable plots

	if {[info exists plots($plotName,Traces)]==0} return
	if {[info exists plots($plotName,Traces,$traceID,Scale)]==0} {
		return -code error "Trace or plot does not exist!"
	}
	
	foreach {option value} $args {
		switch -- $option {
			"-state" {
				if {$value=="on" || $value=="off"} {
					set plots($plotName,Traces,$traceID,State) $value
				} else {
					return -code error {State must be "on" or "off"}
				}
			}
			"-scale" {
				set value [string toupper $value]
				if {$value=="Y1" || $value=="Y2" || $value == "BOTH"} {
					set plots($plotName,Traces,$traceID,Scale) $value
				} else {
					return -code error {scale must be "y1", "y2" or "both"}
				}
			}
			"-color" {
				set plots($plotName,Traces,$traceID,Color) $value
			}
			"-name" {
				if {$value eq {}} {
					return -code error {Name of trace must not be empty}
				}
				set plots($plotName,Traces,$traceID,Name) $value
			}
			"-width" {
				if {[catch {expr {int($value)}}]!=0} {
					return -code error {Width must be a number}
				} elseif {$value < 0} {
					return -code error {Width must be >= 0}
				} else {
					set plots($plotName,Traces,$traceID,Width) [expr {int(0.5+$value)}]
				}
			}
			"-trace" {
				if {[catch {::esweep::size -obj $value}]} {
				       retrurn -code error {Not an esweep object}
			        } else {
					set plots($plotName,Traces,$traceID,Obj) $value
				}
			}

			default { return -code error "Unknown trace option $option"}
		}
	}

	set plots($plotName,Traces,$traceID,Dirty) 1
	set plots($plotName,Config,Dirty) 1
	return true
}

proc ::TkEsweepXYPlot::updateTrace {plotName traceID} {
	variable plots
	if {[info exists plots($plotName,Traces)]} {
		if {[catch {set plots($plotName,Traces,$traceID,Dirty) 1}]==0} {
			::TkEsweepXYPlot::plot $plotName
		}
	}
}

proc ::TkEsweepXYPlot::exists {plotName traceID} {
	variable plots
	if {[info exists plots($plotName,Traces)]} {
		if {[lsearch $plots($plotName,Traces) $traceID] < 0} {
			return 0
		} else {
			return 1
		}
	} else {return 0}
}

proc ::TkEsweepXYPlot::addMarker {plotName traceID n} {
	variable plots
	if {![info exists plots($plotName,Config,Markers)]} {
		return -code error {Plot does not exist!}
	}
	if {[info exists plots($plotName,Traces,$traceID,Scale)]==0} {
		return -code error {Trace does not exist!}
	}
	
	lappend plots($plotName,Traces,$traceID,Markers) $n
	if {$plots($plotName,Config,Markers) eq {on}} {
		set plots($plotName,Config,Dirty) 1
	}
	return [expr {[llength $plots($plotName,Traces,$traceID,Markers)]-1}]
}

proc ::TkEsweepXYPlot::getMarker {plotName traceID {index {}}} {
	variable plots
	if {![info exists plots($plotName,Config,Markers)]} {
		return -code error {Plot does not exist!}
	}
	if {[info exists plots($plotName,Traces,$traceID,Scale)]==0} {
		return -code error {Trace does not exist!}
	}
	if {$index eq {}} {
		return $plots($plotName,Traces,$traceID,Markers)
	} else {
		return [lindex $plots($plotName,Traces,$traceID,Markers) $index]
	}
}

proc ::TkEsweepXYPlot::modifyMarker {plotName traceID index n} {
	variable plots
	if {![info exists plots($plotName,Config,Markers)]} {
		return -code error {Plot does not exist!}
	}
	if {[info exists plots($plotName,Traces,$traceID,Scale)]==0} {
		return -code error {Trace does not exist!}
	}

	lset plots($plotName,Traces,$traceID,Markers) $index $n
	if {$plots($plotName,Config,Markers) eq {on}} {
		set plots($plotName,Config,Dirty) 1
	}
}

proc ::TkEsweepXYPlot::deleteMarker {plotName traceID index} {
	variable plots
	if {![info exists plots($plotName,Config,Markers)]} {
		return -code error {Plot does not exist!}
	}
	if {[info exists plots($plotName,Traces,$traceID,Scale)]==0} {
		return -code error {Trace does not exist!}
	}

	set plots($plotName,Traces,$traceID,Markers) [lreplace \
		$plots($plotName,Traces,$traceID,Markers) $index $index]
	if {$plots($plotName,Config,Markers) eq {on}} {
		set plots($plotName,Config,Dirty) 1
	}
}

#####################
# drawing functions #
#####################

# refresh the complete display
proc ::TkEsweepXYPlot::plot {{plotName {}}} {
	variable plots
	if {$plotName eq {}} {
		set plotName $plots(Names)
	}
	foreach plot $plotName {
		::TkEsweepXYPlot::autoscale $plotName
		::TkEsweepXYPlot::drawTrace $plotName
		if {$plots($plotName,Config,Dirty)} {
			::TkEsweepXYPlot::drawMask $plotName
			::TkEsweepXYPlot::drawCoordSystem $plotName
			::TkEsweepXYPlot::drawTitle $plotName
			::TkEsweepXYPlot::drawCursors $plotName
			#::TkEsweepXYPlot::drawYCursors $plotName
			#::TkEsweepXYPlot::drawReadout $plotName
			::TkEsweepXYPlot::drawLegend $plotName
			::TkEsweepXYPlot::drawMarker $plotName
			set plots($plotName,Config,Dirty) 0
		} 
	}
}

proc ::TkEsweepXYPlot::drawMarker {plotName} {
	variable plots
	variable cData

	if {[info exists plots($plotName,Config,Margin,Left)]==0} {
		return -code error "Plot does not exist!"
	}
	if {$plots($plotName,Config,Markers) ne {on}} {
		return
	}

	set c $cData($plotName,pathName)
	if {![winfo exists $c]} {return false}
	set tag "$plotName,markers"
	# avoid whitespaces in tag
	regsub -all {[\s:]} [string tolower $tag] _ tag 
	catch {$c delete $tag}

	set w [winfo width $c]
	set h [winfo height $c]

	# calculate coordinates; the margins are in percent of canvas width/height
	set x1 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set x2 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]

	set y1 [expr {int(0.5+$h*double($plots($plotName,Config,Margin,Top))/100)}]
	set y2 [expr {$h-int(0.5+$h*double($plots($plotName,Config,Margin,Bottom))/100)}]

	foreach {traceID} $plots($plotName,Traces) {
		if {$plots($plotName,Traces,$traceID,State) == {off}} {continue}
		foreach {i} $plots($plotName,Traces,$traceID,Markers) {
			# get the coordinates
			lassign [esweep::index -obj $plots($plotName,Traces,$traceID,Obj) -index $i] xr y1r y2r
			lassign [real2screen $plotName $plots($plotName,Traces,$traceID,Scale) [list $x1 $y1 $x2 $y2] $xr $y1r] xs ys
			if {$ys > $y2} {set ys $y2} elseif {$ys < $y1} {set ys $y1}
			set coords [list $xs $ys [expr {$xs-4}] [expr {$ys-10}] [expr {$xs+4}] [expr {$ys-10}] $xs $ys]
			$c create line $coords \
				-fill $plots($plotName,Traces,$traceID,Color) \
				-width $plots($plotName,Traces,$traceID,Width) \
				-tag $tag
		}
	}
}

proc ::TkEsweepXYPlot::drawCursors {plotName} {
	variable plots
	variable cData

	if {[info exists plots($plotName,Config,Margin,Left)]==0} {
		return -code error "Plot does not exist!"
	}

	set c $cData($plotName,pathName)
	if {![winfo exists $c]} {return false}
	set tag "$plotName,cursors"
	# avoid whitespaces in tag
	regsub -all {[\s:]} [string tolower $tag] _ tag 
	catch {$c delete $tag}

	set w [winfo width $c]
	set h [winfo height $c]

	# calculate coordinates; the margins are in percent of canvas width/height
	set x1 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set x2 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]

	set y1 [expr {int(0.5+$h*double($plots($plotName,Config,Margin,Top))/100)}]
	set y2 [expr {$h-int(0.5+$h*double($plots($plotName,Config,Margin,Bottom))/100)}]

	# position of the font for the x-scale
	set fontheight [font metrics $plots($plotName,Config,Font) -displayof $c -linespace]
	set fontYPos [expr {$y1-$fontheight}]

	set fmtStr "% -.$plots($plotName,Config,X,Precision)g"
	if {$plots($plotName,Config,Cursors,1) eq {on}} {
		if {$plots($plotName,Config,Cursors,1,X) > $plots($plotName,Config,X,Min) && $plots($plotName,Config,Cursors,1,X) < $plots($plotName,Config,X,Max)} {
			lassign [real2screen $plotName Y1 [list $x1 $y1 $x2 $y2] $plots($plotName,Config,Cursors,1,X) 1] xs ys
			$c create line $xs $y1 $xs $y2 -fill $plots($plotName,Config,Cursors,1,Color) -tag $tag -width $plots($plotName,Config,Cursors,1,Width)
			set tick [format $fmtStr $plots($plotName,Config,Cursors,1,X)]
			$c create text $xs $fontYPos -text $tick -tag $tag -anchor n
		}
	}
	if {$plots($plotName,Config,Cursors,2) eq {on}} {
		if {$plots($plotName,Config,Cursors,2,X) > $plots($plotName,Config,X,Min) && $plots($plotName,Config,Cursors,2,X) < $plots($plotName,Config,X,Max)} {
			lassign [real2screen $plotName Y1 [list $x1 $y1 $x2 $y2] $plots($plotName,Config,Cursors,2,X) 1] xs ys
			$c create line $xs $y1 $xs $y2 -fill $plots($plotName,Config,Cursors,2,Color) -tag $tag -width $plots($plotName,Config,Cursors,2,Width)
			set tick [format $fmtStr $plots($plotName,Config,Cursors,2,X)]
			$c create text $xs $fontYPos -text $tick -tag $tag -anchor n
		}
	}
}

# draw the coordinate system
proc ::TkEsweepXYPlot::drawCoordSystem {plotName} {
	variable plots
	variable cData

	if {[info exists plots($plotName,Config,Margin,Left)]==0} {
		return -code error "Plot does not exist!"
	}
	
	set c $cData($plotName,pathName)
	if {![winfo exists $c]} {return false}
	set tag "$plotName,system"
	# avoid whitespaces in tag
	regsub -all {[\s:]} [string tolower $tag] _ tag 
	catch {$c delete $tag}

	set w [winfo width $c]
	set h [winfo height $c]

	# calculate coordinates; the margins are in percent of canvas width/height
	set x1 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set x2 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]

	set y1 [expr {int(0.5+$h*double($plots($plotName,Config,Margin,Top))/100)}]
	set y2 [expr {$h-int(0.5+$h*double($plots($plotName,Config,Margin,Bottom))/100)}]

	# draw outer rectangle 
	$c create rectangle $x1 $y1 $x2 $y2 -tag $tag -dash $plots($plotName,Config,BorderDash)

	# position of the font for the x-scale
	set fontheight [font metrics $plots($plotName,Config,Font) -displayof $c -linespace]
	set fontYPos [expr {$y2+$fontheight}]

	set fmtStr "% -.$plots($plotName,Config,X,Precision)g"
	# major steps
	if {$plots($plotName,Config,X,Log)!="yes"} {
		set step [expr {double($x2-$x1)/$plots($plotName,Config,X,Step)}]
		set mStep [expr {1.0*$step/$plots($plotName,Config,X,MStep)}]
		set tickStep [expr {double($plots($plotName,Config,X,Max)-$plots($plotName,Config,X,Min))/$plots($plotName,Config,X,Step)}]
		for {set i 1; set x $x1} {$i<$plots($plotName,Config,X,Step)} {incr i} {
			# minor steps
			for {set j 1; set mx [expr {int(0.5+$x+$mStep)}]} {$j < $plots($plotName,Config,X,MStep)} {incr j} {
				$c create line $mx $y1 $mx $y2 -tag $tag -dash $plots($plotName,Config,X,MStepDash)
				set mx [expr {int(0.5+$mx+$mStep)}] 
			}
			set x [expr {int(0.5+$i*$step+$x1)}]
			$c create line $x $y1 $x $y2 -tag $tag -dash $plots($plotName,Config,X,StepDash)
			# draw scale
			set tick [format $fmtStr [expr {$i*$tickStep+$plots($plotName,Config,X,Min)}]]
			$c create text $x $fontYPos -text $tick -tag $tag
		}
		# once more
		for {set j 1; set mx [expr {int(0.5+$x+$mStep)}]} {$j < $plots($plotName,Config,X,MStep)} {incr j} {
			$c create line $mx $y1 $mx $y2 -tag $tag -dash $plots($plotName,Config,X,MStepDash)
			set mx [expr {int(0.5+$mx+$mStep)}] 
		}
	} else {
		# ignore the step variable, determine in which decade we are and draw the lines on each power of 10
		set tickStep 10
		set tick [expr {pow(10, ceil(log10($plots($plotName,Config,X,Min))))}]
		set sc [expr {($x2-$x1)/log10($plots($plotName,Config,X,Max)/$plots($plotName,Config,X,Min))}]
		set mTick [expr {$tick/10}] 
		set mTickStep [expr {$tick/$plots($plotName,Config,X,MStep)}]
		while {$tick<$plots($plotName,Config,X,Max)} {
			for {set i 1} {$i < $plots($plotName,Config,X,MStep)} {incr i} {
				set x [expr {int(0.5+$x1+$sc*log10(($i*$mTickStep+$mTick)/$plots($plotName,Config,X,Min)))}]
				if {$x > $x1 && $x < $x2} {
					$c create line $x $y1 $x $y2 -tag $tag -dash $plots($plotName,Config,X,MStepDash)
				}
			}
			set x [expr {int(0.5+$x1+log10($tick/$plots($plotName,Config,X,Min))*$sc)}]
			$c create line $x $y1 $x $y2 -tag $tag -dash $plots($plotName,Config,X,StepDash) 
			$c create text $x $fontYPos -text [format $fmtStr $tick] -tag $tag
			set mTick $tick
			set tick [expr {$tick*$tickStep}]
			set mTickStep [expr {$tick/$plots($plotName,Config,X,MStep)}]
		}
		# once more ...
		for {set i 1} {$i <= $plots($plotName,Config,X,MStep)} {incr i} {
			set x [expr {int(0.5+$x1+$sc*log10(($i*$mTickStep+$mTick)/$plots($plotName,Config,X,Min)))}]
			if {$x > $x1 && $x < $x2} {
				$c create line $x $y1 $x $y2 -tag $tag -dash $plots($plotName,Config,X,MStepDash)
			}
		}
	}
	# now the scale at the outer edges
	set tick [format $fmtStr $plots($plotName,Config,X,Min)]
	$c create text $x1 $fontYPos -text $tick -tag $tag
	set tick [format $fmtStr $plots($plotName,Config,X,Max)]
	$c create text $x2 $fontYPos -text $tick -tag $tag

	# the label
	set labelXPos [expr {$x1+($x2-$x1)/2}]
	set labelYPos [expr {$y2+2*$fontheight}]
	$c create text $labelXPos $labelYPos -text $plots($plotName,Config,X,Label) -tag $tag

	# position of the font for the y1-scale
	set fontwidth [font measure font -displayof $c "0"] 
	set fontXPos [expr {$x1-$fontwidth}]

	if {$plots($plotName,Config,Y1,State)=="on"} {
		set fmtStr "% -.$plots($plotName,Config,Y1,Precision)g"
		if {$plots($plotName,Config,Y1,Log)!="yes"} {
			set step [expr {double($y2-$y1)/$plots($plotName,Config,Y1,Step)}]
			set mStep [expr {1.0*$step/$plots($plotName,Config,Y1,MStep)}]
			set tickStep [expr {double($plots($plotName,Config,Y1,Max)-$plots($plotName,Config,Y1,Min))/$plots($plotName,Config,Y1,Step)}]
			for {set i 1; set y $y2} {$i<$plots($plotName,Config,Y1,Step)} {incr i} {
				# minor steps
				for {set j 1; set my [expr {int(0.5+$y-$mStep)}]} {$j < $plots($plotName,Config,Y1,MStep)} {incr j} {
					$c create line $x1 $my $x2 $my -tag $tag -dash $plots($plotName,Config,Y1,MStepDash)
					set my [expr {int(0.5+$my-$mStep)}] 
				}
				set y [expr {int(0.5+$y2-$i*$step)}]
				$c create line $x1 $y $x2 $y -tag $tag
				# draw scale
				set tick [format $fmtStr [expr {$i*$tickStep+$plots($plotName,Config,Y1,Min)}]]
				$c create text $fontXPos $y -text $tick -anchor e -tag $tag
			}
			# once more
			for {set j 1; set my [expr {int(0.5+$y-$mStep)}]} {$j < $plots($plotName,Config,Y1,MStep)} {incr j} {
				$c create line $x1 $my $x2 $my -tag $tag -dash $plots($plotName,Config,Y1,MStepDash)
				set my [expr {int(0.5+$my-$mStep)}] 
			}
		} else {
			# ignore the step variable, determine in which decade we are and draw the lines on each power of 10
			set tickStep 10
			set tick [expr {pow(10, ceil(log10($plots($plotName,Config,Y1,Min))))}]
			set sc [expr {($y2-$y1)/log10($plots($plotName,Config,Y1,Max)/$plots($plotName,Config,Y1,Min))}]
			set mTick [expr {$tick/10}] 
			set mTickStep [expr {$tick/$plots($plotName,Config,Y1,MStep)}]
			while {$tick<$plots($plotName,Config,Y1,Max)} {
				for {set i 1} {$i < $plots($plotName,Config,Y1,MStep)} {incr i} {
					set y [expr {int(0.5+$y2-$sc*log10(($i*$mTickStep+$mTick)/$plots($plotName,Config,Y1,Min)))}]
					if {$y > $y1 && $y < $y2} {
						$c create line $x1 $y $x2 $y -tag $tag -dash $plots($plotName,Config,Y1,MStepDash)
					}
				}
				set y [expr {int(0.5+$y2-log10($tick/$plots($plotName,Config,Y1,Min))*$sc)}]
				$c create line $x1 $y $x2 $y -tag $tag
				$c create text $fontXPos $y -text [format $fmtStr $tick] -anchor e -tag $tag
				set mTick [expr {pow(10, floor(log10($plots($plotName,Config,Y1,Min))))}] 
				set tick [expr {$tick*$tickStep}]
				set mTickStep [expr {$tick/$plots($plotName,Config,Y1,MStep)}]
			}
			# once more ...
			for {set i 1} {$i < $plots($plotName,Config,Y1,MStep)} {incr i} {
				set y [expr {int(0.5+$y2-$sc*log10(($i*$mTickStep+$mTick)/$plots($plotName,Config,Y1,Min)))}]
				if {$y > $y1 && $y < $y2} {
					$c create line $x1 $y $x2 $y -tag $tag -dash $plots($plotName,Config,Y1,MStepDash)
				}
			}
		}
		# now the scale at the outer edges
		set tick [format $fmtStr $plots($plotName,Config,Y1,Min)]
		$c create text $fontXPos $y2 -text $tick -anchor e -tag $tag
		set tick [format $fmtStr $plots($plotName,Config,Y1,Max)]
		$c create text $fontXPos $y1 -text $tick -anchor e -tag $tag

		# the label
		# as a Tk Canvas cannot rotate individual elements, we position it on top of the axis
		set labelXPos $x1
		set labelYPos [expr {$y1-1*$fontheight}]
		$c create text $labelXPos $labelYPos -text $plots($plotName,Config,Y1,Label) -tag $tag
	}

	set fontXPos [expr {$x2+$fontwidth}]
	if {$plots($plotName,Config,Y2,State)=="on"} {
		set fmtStr "% -.$plots($plotName,Config,Y2,Precision)g"
		if {$plots($plotName,Config,Y2,Log)!="yes"} {
			set step [expr {double($y2-$y1)/$plots($plotName,Config,Y2,Step)}]
			set mStep [expr {1.0*$step/$plots($plotName,Config,Y1,MStep)}]
			set tickStep [expr {double($plots($plotName,Config,Y2,Max)-$plots($plotName,Config,Y2,Min))/$plots($plotName,Config,Y2,Step)}]
			for {set i 1; set y $y2} {$i<$plots($plotName,Config,Y2,Step)} {incr i} {
				# minor steps
				for {set j 1; set my [expr {int(0.5+$y-$mStep)}]} {$j < $plots($plotName,Config,Y2,MStep)} {incr j} {
					$c create line $x1 $my $x2 $my -tag $tag -dash $plots($plotName,Config,Y2,MStepDash)
					set my [expr {int(0.5+$my-$mStep)}] 
				}
				set y [expr {int(0.5+$y2-$i*$step)}]
				$c create line $x1 $y $x2 $y -tag $tag
				# draw scale
				set tick [format $fmtStr [expr {$i*$tickStep+$plots($plotName,Config,Y2,Min)}]]
				$c create text $fontXPos $y -text $tick -anchor w -tag $tag
			}
			# minor steps
			for {set j 1; set my [expr {int(0.5+$y-$mStep)}]} {$j < $plots($plotName,Config,Y2,MStep)} {incr j} {
				$c create line $x1 $my $x2 $my -tag $tag -dash $plots($plotName,Config,Y2,MStepDash)
				set my [expr {int(0.5+$my-$mStep)}] 
			}
		} else {
			# ignore the step variable, determine in which decade we are and draw the lines on each power of 10
			set tickStep 10
			set tick [expr {pow(10, ceil(log10($plots($plotName,Config,Y2,Min))))}]
			set sc [expr {($y2-$y1)/log10($plots($plotName,Config,Y2,Max)/$plots($plotName,Config,Y2,Min))}]
			set mTick [expr {$tick/10}] 
			set mTickStep [expr {$tick/$plots($plotName,Config,Y1,MStep)}]
			while {$tick<$plots($plotName,Config,Y2,Max)} {
				for {set i 1} {$i < $plots($plotName,Config,Y2,MStep)} {incr i} {
					set y [expr {int(0.5+$y2-$sc*log10(($i*$mTickStep+$mTick)/$plots($plotName,Config,Y2,Min)))}]
					if {$y > $y1 && $y < $y2} {
						$c create line $x1 $y $x2 $y -tag $tag -dash $plots($plotName,Config,Y2,MStepDash)
					}
				}
				set y [expr {int(0.5+$y2-log10($tick/$plots($plotName,Config,Y2,Min))*$sc)}]
				$c create line $x1 $y $x2 $y -tag $tag
				$c create text $fontXPos $y -text [format $fmtStr $tick] -anchor w -tag $tag
				set mTick [expr {pow(10, floor(log10($plots($plotName,Config,Y2,Min))))}] 
				set tick [expr {$tick*$tickStep}]
				set mTickStep [expr {$tick/$plots($plotName,Config,Y2,MStep)}]
			}
			# once more ...
			for {set i 1} {$i < $plots($plotName,Config,Y2,MStep)} {incr i} {
				set y [expr {int(0.5+$y2-$sc*log10(($i*$mTickStep+$mTick)/$plots($plotName,Config,Y2,Min)))}]
				if {$y > $y1 && $y < $y2} {
					$c create line $x1 $y $x2 $y -tag $tag -dash $plots($plotName,Config,Y2,MStepDash)
				}
			}
		}
		# now the scale at the outer edges
		set tick [format $fmtStr $plots($plotName,Config,Y2,Min)]
		$c create text $fontXPos $y2 -text $tick -anchor w -tag $tag
		set tick [format $fmtStr $plots($plotName,Config,Y2,Max)]
		$c create text $fontXPos $y1 -text $tick -anchor w -tag $tag

		# the label
		# as a Tk Canvas cannot rotate individual elements, we position it on top of the axis
		set labelXPos $x2
		set labelYPos [expr {$y1-2*$fontheight}]
		$c create text $labelXPos $labelYPos -text $plots($plotName,Config,Y2,Label) -tag $tag
	}
}

# draw a mask which hides the drawed lines outside the drawing area
proc ::TkEsweepXYPlot::drawMask {plotName} {
	variable cData
	variable plots

	if {[info exists plots($plotName,Config,Margin,Left)]==0} {
		return -code error "Plot does not exist!"
	}

	set c $cData($plotName,pathName)
	set tag "$plotName,mask"
	# avoid whitespaces in tag
	regsub -all {[\s:]} [string tolower $tag] _ tag 
	catch {$c delete $tag}
	set w [winfo width $c]
	set h [winfo height $c]
	# left mask
	set x1 0
	set x2 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set y1 0
	set y2 $h 
	$c create rectangle $x1 $y1 $x2 $y2 -fill $cData($plotName,bgColor) -outline $cData($plotName,bgColor) -tags $tag

	# top mask
	set x1 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set x2 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]
	set y1 0
	set y2 [expr {int(0.5+$h*double($plots($plotName,Config,Margin,Top))/100)}]
	$c create rectangle $x1 $y1 $x2 $y2 -fill $cData($plotName,bgColor) -outline $cData($plotName,bgColor) -tags $tag

	# right mask
	set x1 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]
	set x2 $w
	set y1 0
	set y2 $h 
	$c create rectangle $x1 $y1 $x2 $y2 -fill $cData($plotName,bgColor) -outline $cData($plotName,bgColor) -tags $tag

	# bottom mask
	set x1 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set x2 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]
	set y1 [expr {$h-int(0.5+$h*double($plots($plotName,Config,Margin,Bottom))/100)}]
	set y2 $h
	$c create rectangle $x1 $y1 $x2 $y2 -fill $cData($plotName,bgColor) -outline $cData($plotName,bgColor) -tags $tag
}

proc ::TkEsweepXYPlot::drawLegend {plotName} {
	variable plots
	variable cData

	# delete old legend anyways
	set c $cData($plotName,pathName)
	set tag "$plotName,legend"
	# avoid whitespaces in tag
	regsub -all {[\s:]} [string tolower $tag] _ tag 
	catch {$c delete $tag}

	# do not draw a new legend if no trace exists
	if {[info exists plots($plotName,Traces)]==0 || [llength $plots($plotName,Traces)]<=0} return

	# calculate outer rectangle coordinates; the margins are in percent of canvas width/height
	# the legend will not exceed the size of the rectangle
	set w [winfo width $c]
	set h [winfo height $c]
	set x1 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set x2 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]

	set y1 [expr {int(0.5+$h*double($plots($plotName,Config,Margin,Top))/100)}]
	set y2 [expr {$h-int(0.5+$h*double($plots($plotName,Config,Margin,Bottom))/100)}]

	switch $plots($plotName,Config,Legend,Position) {
		top {
		}
		bottom {
			::TkEsweepXYPlot::drawLegendBottom $plotName $x1 $x2 $y1 $y2 $tag 
		}
		left {
		}
		right {
		}
		default {
		}
	}
}

proc ::TkEsweepXYPlot::drawLegendBottom {plotName x1 x2 y1 y2 tag} {
	variable plots
	variable cData
	
	set c $cData($plotName,pathName)
	set fontHeight [font metrics $plots($plotName,Config,Font) -displayof $c -linespace]

	set symbolLength $plots($plotName,Config,Legend,Symbol,Length)
	set borderWidth $plots($plotName,Config,Legend,Border,Width)

	set top [expr {$y2+3*$fontHeight}]
	set yL $top
	set xL $x1
	set xmax $xL

	foreach traceID $plots($plotName,Traces) {
		set color $plots($plotName,Traces,$traceID,Color)
		set pen $plots($plotName,Traces,$traceID,Width)
		set textWidth [font measure font -displayof $c $plots($plotName,Traces,$traceID,Name)] 

		# calculate the length of the legend entry
		# 2*$borderwidth is used because of the border on the left side of the entry
		# and the border between symbol and traceID
		# the border behind the entry is added to the following entry
		set length [expr {2*$borderWidth+$symbolLength+$textWidth}]
		# line feed
		if {$xL+$length > $x2} {
			set xL $x1
			set yL [expr {$yL+$fontHeight}]
			if {$xL > $xmax} {set xmax $xL}
		} 

		set coords {}
		set xL [expr {$xL+$borderWidth}]
		lappend coords $xL
		lappend coords [expr {$yL+$fontHeight/2}]
		set xL [expr {$xL+$symbolLength}]
		lappend coords $xL
		lappend coords [expr {$yL+$fontHeight/2}]

		$c create line $coords -fill $plots($plotName,Traces,$traceID,Color) -tag $tag -width $plots($plotName,Traces,$traceID,Width)
		set xL [expr {$xL+$borderWidth}]
		$c create text $xL $yL -text $plots($plotName,Traces,$traceID,Name) -tag $tag -anchor nw
		set xL [expr {$xL+$textWidth}]
		if {$xL > $xmax} {set xmax $xL}
	}

	# draw rectangle around the legend
	$c create rectangle $x1 $top [expr {$xmax+$borderWidth}] [expr {$yL+$fontHeight}] -tag $tag
	# alignment
	switch $plots($plotName,Config,Legend,Align) {
		left {
			set xshift 0
		}
		right {
			set xshift [expr {$x2-($xmax+$borderWidth)}]
		}
		center {
			set xshift [expr {($x2+$x1)/2-($xmax+$borderWidth+$x1)/2}]
		}
		default {
			set xshift 0
		}
	}
	$c move $tag $xshift 0
}

proc ::TkEsweepXYPlot::drawTitle {plotName} {
	variable plots
	variable cData

	set c $cData($plotName,pathName)
	set tag "$plotName,title"
	# avoid whitespaces in tag
	regsub -all {[\s:]} [string tolower $tag] _ tag 
	$c delete $tag

	set fontheight [font metrics $plots($plotName,Config,Font) -displayof $c -linespace]

	# draw the title
	set titleXPos [expr {[winfo width $c]/2}]
	set titleYPos [expr {$fontheight}]
	$c create text $titleXPos $titleYPos -text $plots($plotName,Config,Title) -tag $tag
}

# draw the named trace; if $name == "" then draw all; this is the default
proc ::TkEsweepXYPlot::drawTrace {plotName {name ""}} {
	variable cData
	variable plots

	if {[info exists plots($plotName,Config,Margin,Left)]==0} {
		return -code error "Plot does not exist!"
	}

	if {[info exists plots($plotName,Traces)]==0} {
		# no traces defined yet
		return
	}
	
	set c $cData($plotName,pathName)

	if {$name==""} {
		set names *
	} else {
		set names $name
	}

	set w [winfo width $c]
	set h [winfo height $c]

	# calculate coordinates of the outer rectangle; the margins are in percent of canvas width/height
	set x1 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set x2 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]

	set y1 [expr {int(0.5+$h*double($plots($plotName,Config,Margin,Top))/100)}]
	set y2 [expr {$h-int(0.5+$h*double($plots($plotName,Config,Margin,Bottom))/100)}]
	set screen [list $x1 $y1 $x2 $y2]

	set xmin $plots($plotName,Config,X,Min)
	set xmax $plots($plotName,Config,X,Max)

	set coordList [list]
	foreach {traceID} $plots($plotName,Traces) {
		if {$plots($plotName,Traces,$traceID,Dirty)==0 && $plots($plotName,Config,Dirty)==0} { 
			continue
		} else {
			set plots($plotName,Traces,$traceID,Dirty) 0
		}

		if {$plots($plotName,Traces,$traceID,State) == {off}} {
			if {![catch {$c delete $plots($plotName,Traces,$traceID,ID1)}]} {
				set plots($plotName,Traces,$traceID,ID1) {}
			}
			if {![catch {$c delete $plots($plotName,Traces,$traceID,ID2)}]} {
				set plots($plotName,Traces,$traceID,ID2) {}
			}
			continue
		}

		if {$plots($plotName,Traces,$traceID,Scale) == {Y1} \
			|| $plots($plotName,Traces,$traceID,Scale) == {BOTH} } {
			set world [list $xmin \
					$plots($plotName,Config,Y1,Min) \
					$xmax \
					$plots($plotName,Config,Y1,Max)]
			set scale {abs}
			if {$plots($plotName,Config,X,Log)=={yes}} {set log {x}} else {set log {}}
			if {$plots($plotName,Config,Y1,Log)=={yes}} {append log {y}}

			set coordList [esweep::getCoords -obj $plots($plotName,Traces,$traceID,Obj) \
							 -screen $screen \
							 -world $world \
							 -log $log \
							 -opt $scale]

			if {$plots($plotName,Traces,$traceID,ID1) eq {}} {
				set plots($plotName,Traces,$traceID,ID1) [$c create line $coordList \
					-width $plots($plotName,Traces,$traceID,Width) \
					-fill $plots($plotName,Traces,$traceID,Color)]
			} else {
				$c coords $plots($plotName,Traces,$traceID,ID1) $coordList
			}
			$c lower $plots($plotName,Traces,$traceID,ID1)
		}
		if {$plots($plotName,Traces,$traceID,Scale) == {Y2} \
			|| $plots($plotName,Traces,$traceID,Scale) == {BOTH} } {
			set world [list $xmin \
					$plots($plotName,Config,Y2,Min) \
					$xmax \
					$plots($plotName,Config,Y2,Max)]
			set scale {arg}
			if {$plots($plotName,Config,X,Log)=={yes}} {set log {x}} else {set log {}}
			if {$plots($plotName,Config,Y2,Log)=={yes}} {append log {y}}

			set coordList [esweep::getCoords -obj $plots($plotName,Traces,$traceID,Obj) \
							 -screen $screen \
							 -world $world \
							 -log $log \
							 -opt $scale]
			if {$plots($plotName,Traces,$traceID,ID2) eq {}} {
				set plots($plotName,Traces,$traceID,ID2) [$c create line $coordList \
					-width $plots($plotName,Traces,$traceID,Width) \
					-fill $plots($plotName,Traces,$traceID,Color)]
					-dash . 
			} else {
				$c coords $plots($plotName,Traces,$traceID,ID2) $coordList
			}
			$c lower $plots($plotName,Traces,$traceID,ID2)
		}
	}
}

# draw the cursors
proc ::TkEsweepXYPlot::drawXCursors {plotName} {
}

proc ::TkEsweepXYPlot::drawYCursors {plotName} {

}

proc ::TkEsweepXYPlot::drawReadout {plotName} {

}

proc ::TkEsweepXYPlot::autoscale {plotName {scale {all}}} {
	variable plots

	if {[info exists plots($plotName,Traces)]==0} {
		# no traces defined yet
		return
	}

	if {$scale=={all}} {set sc {x y1 y2}} else {set sc $scale}

	foreach s [split [string toupper $sc]] {
		if {$plots($plotName,Config,$s,Autoscale)=={off}} {continue}
		if {$plots($plotName,Config,$s,State)=={off}} {continue}
		set plots($plotName,Config,Dirty) 1
		set max inf
		set min -inf
		switch $s {
			Y2 -
			Y1 {
				foreach {traceID} $plots($plotName,Traces) {
					set obj $plots($plotName,Traces,$traceID,Obj)
					switch [esweep::type -obj $obj] {
						wave -
						complex {
							set df [expr {1000.0/[esweep::samplerate -obj $obj]}]
							set from [expr {int(0.5+$plots($plotName,Config,X,Min)/$df)}]
							set to [expr {int(0.5+$plots($plotName,Config,X,Max)/$df)}]
						}
						polar -
						default {
							set df [expr {1.0*[esweep::samplerate -obj $obj]/[esweep::size -obj $obj]}]
							set from [expr {int(0.5+$plots($plotName,Config,X,Min)/$df)}]
							set to [expr {int(0.5+$plots($plotName,Config,X,Max)/$df)}]
						}
					}
					# if Xmax > size use the whole range
					set to [expr {$to >= [esweep::size -obj $obj] ? -1 : $to}]
					if {$max == {inf}} {
						set max [esweep::max -obj $obj -from $from -to $to]
					} else {
						set m [esweep::max -obj $obj -from $from -to $to]
						if {$m > $max} {set max $m}
					}
					if {$min == {-inf}} {
						set min [esweep::min -obj $obj -from $from -to $to]
					} else {
						set m [esweep::min -obj $obj -from $from -to $to]
						if {$m < $min} {set min $m}
					}
				}
			}
			X {
				foreach {traceID} $plots($plotName,Traces) {
					set obj $plots($plotName,Traces,$traceID,Obj)
					set min 0
					if {$max == {inf}} {
						set max [expr {1000.0*[esweep::size -obj $obj]/[esweep::samplerate -obj $obj]}]
					} else {
						set m [expr {1000.0*[esweep::size -obj $obj]/[esweep::samplerate -obj $obj]}]
						if {$m > $max} {set max $m}
					}
				}
			}

			default {}
		}
		if {$max eq {inf}} {
			set max 1e10
		} elseif {$max eq {-inf}} {set max 1e-9}
		if {$min eq {inf}} {
			set min 1e9
		} elseif {$min eq {-inf}} {set min 1e-10}
		# should never happen, but who knows? 
		if {$max < $min} {
			set m $min
			set min $max
			set max $m
		}

		if {$plots($plotName,Config,$s,Log)=={yes}} {
			if {$min<1e-10} {
				set min 1e-10
			} else {
				set dec [expr {floor(log10(abs($min)))}]
				set min [expr {pow(10, $dec)}]
			}
			if {$max < 1e-9} {
				set max 1e-9
			} else {
				set dec [expr {ceil(log10(abs($max)))}]
				set max [expr {pow(10, $dec)}]
			}
		} else {
			if {abs($max) < 1e-9} {
				set max 0
			} else {
				if {$s ne {X}} {
					set decade [expr {pow(10, floor(log10(abs($max)))-1)}]
					set max [expr {ceil($max/$decade)*$decade}]
				}
			}

			if {abs($min) < 1e-9} {
				set min 0
			} else {
				if {$s ne {X}} {
					set decade [expr {pow(10, floor(log10(abs($min)))-1)}]
					set min [expr {floor($min/$decade)*$decade}]
				}
			}
		}

		if {$plots($plotName,Config,$s,Range) ne {}} {
			# adjust max so that Y?,Steps fit smoothly in the graph
			set div [expr {$plots($plotName,Config,$s,Range)/$plots($plotName,Config,$s,Step)}]
			set max [expr {ceil($max/$div)*$div}]
			set min [expr {$max-$plots($plotName,Config,$s,Range)}]
		} else {
			if {$plots($plotName,Config,$s,Bounds) ne {}} {
				if {$min < [lindex $plots($plotName,Config,$s,Bounds) 0]} {
					set min [lindex $plots($plotName,Config,$s,Bounds) 0]
				}
				if {$max > [lindex $plots($plotName,Config,$s,Bounds) 1]} {
					set max [lindex $plots($plotName,Config,$s,Bounds) 1]
				}
			}
		}
		set plots($plotName,Config,$s,Min) $min
		set plots($plotName,Config,$s,Max) $max
	}
}

proc ::TkEsweepXYPlot::mouseHoverReadout {plotName x y} {
	variable plots
	variable events
	variable cData


	set c $cData($plotName,pathName)
	set tag "$plotName,hover"
	catch {$c delete $tag}
	catch {after cancel $events(MouseHoverDelete)}

	if {$plots($plotName,Config,MouseHoverActive)!="yes"} return

	set w [winfo width $c]
	set h [winfo height $c]

	# calculate coordinates of the outer rectangle; the margins are in percent of canvas width/height
	set x1 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set x2 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]

	set y1 [expr {int(0.5+$h*double($plots($plotName,Config,Margin,Top))/100)}]
	set y2 [expr {$h-int(0.5+$h*double($plots($plotName,Config,Margin,Bottom))/100)}]

	# display only when inside the graph
	if {$x < $x1 || $x > $x2 || $y < $y1 || $y > $y2} return

	set fontHeight [font metrics $plots($plotName,Config,Font) -displayof $c -linespace]
	# shift the display of the readout so it will not be obscured by the mouse pointer
	set xshift [expr {$x+20}]
	# convert screen to real coordinates
	if {$plots($plotName,Config,X,State)=="on"} {
		set fmtStr "% -.$plots($plotName,Config,X,Precision)g"
		if {$plots($plotName,Config,X,Log)!="yes"} {
			set xscale [expr {double($x2-$x1)/($plots($plotName,Config,X,Max)-$plots($plotName,Config,X,Min))}]
			set cx [expr {($x-$x1)/$xscale+$plots($plotName,Config,X,Min)}]
		} else {
			set xscale [expr {($x2-$x1)/log10($plots($plotName,Config,X,Max)/$plots($plotName,Config,X,Min))}]
			set cx [expr {pow(10, ($x-$x1)/$xscale)*$plots($plotName,Config,X,Min)}]
		}
		set readout "X: [format $fmtStr $cx]"
		$c create text $xshift $y -text $readout -anchor nw -tag $tag
		set by [expr {$y+$fontHeight}]
		set bx [expr {$xshift+[font measure $plots($plotName,Config,Font) -displayof $c $readout]}]
	}
	if {$plots($plotName,Config,Y1,State)=="on"} {
		set fmtStr "% -.$plots($plotName,Config,Y1,Precision)g"
		if {$plots($plotName,Config,Y1,Log)!="yes"} {
			set y1scale [expr {double($y2-$y1)/($plots($plotName,Config,Y1,Max)-$plots($plotName,Config,Y1,Min))}]
			set cy1 [expr {$plots($plotName,Config,Y1,Max)-($y-$y1)/$y1scale}]
		} else {
			set y1scale [expr {($y2-$y1)/log10($plots($plotName,Config,Y1,Max)/$plots($plotName,Config,Y1,Min))}]
			set cy1 [expr {pow(10, ($y2-$y)/$y1scale)*$plots($plotName,Config,Y1,Min)}]
		}
		set readout "Y1: [format $fmtStr $cy1]"
		$c create text $xshift $by -text $readout -anchor nw -tag $tag
		set by [expr {$by+$fontHeight}]
		set bxn [expr {$xshift+[font measure $plots($plotName,Config,Font) -displayof $c $readout]}]
		set bx [expr {$bxn > $bx ? $bxn : $bx}]
	}
	if {$plots($plotName,Config,Y2,State)=="on"} {
		set fmtStr "% -.$plots($plotName,Config,Y2,Precision)g"
		if {$plots($plotName,Config,Y2,Log)!="yes"} {
			set y2scale [expr {double($y2-$y1)/($plots($plotName,Config,Y2,Max)-$plots($plotName,Config,Y2,Min))}]
			set cy2 [expr {$plots($plotName,Config,Y2,Max)-($y-$y1)/$y1scale}]
		} else {
			set y2scale [expr {($y2-$y1)/log10($plots($plotName,Config,Y2,Max)/$plots($plotName,Config,Y2,Min))}]
			set cy2 [expr {pow(10, ($y2-$y)/$y2scale)*$plots($plotName,Config,Y2,Min)}]
		}
		set readout "Y2: [format $fmtStr $cy2]"
		$c create text $xshift $by -text $readout -anchor nw -tag $tag
		set by [expr {$by+$fontHeight}]
		set bxn [expr {$xshift+[font measure $plots($plotName,Config,Font) -displayof $c $readout]}]
		set bx [expr {$bxn > $bx ? $bxn : $bx}]
	}
	# draw bounding box
	incr xshift -2
	$c create rectangle $xshift $y $bx $by -tag $tag,box -fill yellow
	$c lower $tag,box $tag
	$c itemconfigure $tag,box -tag $tag
	set events(MouseHoverDelete) [after $plots($plotName,Config,MouseHoverDeleteDelay) [list $c delete $tag]]
}

proc ::TkEsweepXYPlot::getCursor {plotName cursor} {
	variable plots
	if {$plots($plotName,Config,Cursors,$cursor)!={on}} {
		return {}
	} else {
		return $plots($plotName,Config,Cursors,$cursor,X)
	}
}

proc ::TkEsweepXYPlot::setCursor {plotName cursor x {y {}}} {
	variable plots
	variable cData

	set c $cData($plotName,pathName)

	if {$plots($plotName,Config,Cursors,1) != {on} && $cursor == 1} return
	if {$plots($plotName,Config,Cursors,2) != {on} && $cursor == 2} return

	set w [winfo width $c]
	set h [winfo height $c]

	# calculate coordinates of the outer rectangle; the margins are in percent of canvas width/height
	set x1 [expr {int(0.5+$w*double($plots($plotName,Config,Margin,Left))/100)}]
	set x2 [expr {$w-int(0.5+$w*double($plots($plotName,Config,Margin,Right))/100)}]

	set y1 [expr {int(0.5+$h*double($plots($plotName,Config,Margin,Top))/100)}]
	set y2 [expr {$h-int(0.5+$h*double($plots($plotName,Config,Margin,Bottom))/100)}]

	# only when inside the graph
	if {$y eq {}} {
		if {$x < $x1 || $x > $x2} return
	} else {
		if {$x < $x1 || $x > $x2 || $y < $y1 || $y > $y2} return
	}

	set border [list $x1 $y1 $x2 $y2]
	if {$cursor == 1} {set button 1} elseif {$cursor == 2} {set button 3} else {set button 2}
	bind $c <Motion> [list ::TkEsweepXYPlot::moveCursor $c $plotName $border $cursor $button %x %y]
	bind $c <ButtonRelease-$button> [list ::TkEsweepXYPlot::fixCursor $c $plotName $button]

	lassign [screen2real $plotName Y1 $border $x $y] xr yr
	set plots($plotName,Config,Cursors,$cursor,X) $xr
	drawCursors $plotName
}

proc ::TkEsweepXYPlot::moveCursor {c plotName border cursor button x y} {
	variable plots
	lassign [screen2real $plotName Y1 $border $x $y] xr yr
	set plots($plotName,Config,Cursors,$cursor,X) $xr
	drawCursors $plotName
}

proc ::TkEsweepXYPlot::fixCursor {c plotName button} {
	bind $c <ButtonRelease-$button> {}
	bind $c <Motion> [list ::TkEsweepXYPlot::mouseHoverReadout $plotName %x %y]
}

# private function, converts screen to real coordinates
proc ::TkEsweepXYPlot::screen2real {plotName scale border xs ys} {
	variable plots
	lassign $border x1 y1 x2 y2
	if {$plots($plotName,Config,X,Log)!={yes}} {
		set xscale [expr {double($x2-$x1)/($plots($plotName,Config,X,Max)-$plots($plotName,Config,X,Min))}]
		set xr [expr {($xs-$x1)/$xscale+$plots($plotName,Config,X,Min)}]
	} else {
		set xscale [expr {($x2-$x1)/log10($plots($plotName,Config,X,Max)/$plots($plotName,Config,X,Min))}]
		set xr [expr {pow(10, ($xs-$x1)/$xscale)*$plots($plotName,Config,X,Min)}]
	}
	if {$plots($plotName,Config,$scale,Log)!={yes}} {
		set yscale [expr {double($y2-$y1)/($plots($plotName,Config,$scale,Max)-$plots($plotName,Config,$scale,Min))}]
		set yr [expr {$plots($plotName,Config,Y1,Max)-($ys-$y1)/$yscale}]
	} else {
		set yscale [expr {($y2-$y1)/log10($plots($plotName,Config,$scale,Max)/$plots($plotName,Config,$scale,Min))}]
		set yr [expr {pow(10, ($y2-$ys)/$yscale)*$plots($plotName,Config,$scale,Min)}]
	}
	return [list $xr $yr]
}

# private function, converts real to screen coordinates
proc ::TkEsweepXYPlot::real2screen {plotName scale border xr yr} {
	variable plots
	lassign $border x1 y1 x2 y2

	set xmin $plots($plotName,Config,X,Min)
	set xmax $plots($plotName,Config,X,Max)
	set ymin $plots($plotName,Config,$scale,Min)
	set ymax $plots($plotName,Config,$scale,Max)
	if {$plots($plotName,Config,X,Log)!={yes}} {
		set xscale [expr {double($x2-$x1)/($xmax-$xmin)}]
		set xs [expr {$xscale*($xr-$xmin)+$x1}]	
	} else {
		set xscale [expr {($x2-$x1)/log10($xmax/$xmin)}]
		set xs [expr {$xscale*log10($xr/$xmin)+$x1}]
	}
	if {$plots($plotName,Config,$scale,Log)!={yes}} {
		set yscale [expr {double($y2-$y1)/($ymax-$ymin)}]
		set ys [expr {$yscale*($ymax-$yr)+$y1}]	
	} else {
		set yscale [expr {double($y2-$y1)/($ymax-$ymin)}]
		set ys [expr {$yscale*($ymax-$yr)+$y1}]	
	}

	return [list $xs $ys]
}

if {[catch {package require Img}]==0} {
	proc ::TkEsweepXYPlot::toImage {plotName} {
		variable plots
		variable cData
		variable events

		set c $cData($plotName,pathName)

		# remove mouse hover readout
		catch {after cancel $events(MouseHoverDelete)}
		catch {$c delete "$plotName,hover"}

		update 
		set im [image create photo ::TkEsweepXYPlot::$plotName,img -data $c]
		
		return $im
	}
} else {
	proc ::TkEsweepXYPlot::toImage {plotName} {
		tk_messageBox -message "Package Img not found. Can't create hardcopy." -type ok
		return ""
	}
}


