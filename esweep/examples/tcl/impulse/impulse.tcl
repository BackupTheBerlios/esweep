#!/usr/local/bin/tclsh8.5
package require Tk 8.5
package require Ttk 8.5

load ../esweep.so; # replace this line by something like package require ...
source ../TkEsweepXYPlot/TkEsweepXYPlot.tcl

############
# Overview #
############

# This program measures the impulse response of a system. 
# Its main purpose is the measurement of speakers during the development process, 
# but it is not limited to this. 
# The program is kept as simple as possible, so there is not much eyecandy or comfort. 
# The only features are those of which I think they are useful. 
# 
# The configuration is very static, i. e. there is the following default configuration, 
# which is also written to a file. 
# The configuration has a simple form: {Part,?Subpart?,Token Value}. 
# User definable parts are: Audio, Mic, Signal, Input, Output, Processing, View
# There is also the Intern part, which stores some runtime information, and is neither read from
# nor written to the configuration file
# Note the strange comment style. This is a way of using comments in "array set"; 

namespace eval impulse {
	variable config
	array set config {
		# {Audio device}
		Audio,Device {audio:/dev/audio}
		# {Samplerate}
		Audio,Samplerate 48000
		# {Mixer device. This program reads out the mixer state and saves it in an additional file. \
		   At each STARTUP of the program it resets the mixer to the saved values}
		Audio,Mixer {/dev/mixer1}
		# {bitrate}
		Audio,Bitdepth 16
		# {Microphone sensitivity in V/Pa}
		Mic,Global,Sensitivity 0.1
		# {Microphone frequency response compensation file \
		   If empty, no compensation is done}
		Mic,Global,Compensation ""
		# {Microphone distance, will be removed from the impulse response} 
		# {for maximum frequency resolution.}
		# {If the distance is < 0, then the program searches for the highest peak}
		Mic,Global,Distance 1
		# {output channel}
		Output,Channel	1 
		# {output level in Volt}
		Output,Global,Level	2.0 
		# {gain of external amplifier}
		Output,Global,Gain 1.0
		# {Maximum output level in 1/FS. If < 1 avoids oversteering}
		Output,MaxFSLevel 0.9 
		# {Calibration level for the output in V/FS (RMS!)}
		Output,Global,Cal	2.0 
		# {Signal type}
		Signal,Type logsweep
		# {Signal duration in ms}
		Signal,Duration 2000
		# {Signal spectrum}
		Signal,Spectrum pink
		# {Signal low cutoff in Hz}
		Signal,Locut 20
		# {Signal high cutoff in Hz}
		Signal,Hicut 20000
		# {Main input channel} 
		Input,Channel	1
		# {Reference channel}
		Input,Reference 2
		# {gain of external amplifier}
		Input,Global,Gain 1.0
		# {Calibration level for the input in V/FS (RMS!)}
		Input,Global,Cal 2.0
		# {Length of gate in ms}
		Processing,Gate 100
		# {Length of FFT in samples}
		# {if 0 then the length is automatically selected}
		Processing,FFT,Size 0
		# {The scaling can be either Relative (SPL/Pa/V)}
		# {or Absolute (SPL)}
		Processing,Scaling Relative
		# {The reference level}
		Processing,Scaling,Reference 2.83
		# {Smoothing factor}
		Processing,Smoothing 6

		# {Internal part}
		
		# {Number of curves displayed in the diff window}
		Intern,MaxOverlays 10
		# {hold the typed numbers for e. g. 12dd}
		Intern,numBuffer {}
		# {Is 1 if there are unsaved changes}
		Intern,Unsaved 0
		# {Store the filename}
		Intern,Filename {}
	}
	# delete comments from array
	array unset config {#}

	proc configure {opt val} {
		variable config
		if {[info exists config($opt)]} {
			set config($opt) $val
		}
	}

	proc cget {opt} {
		variable config
		if {[info exists config($opt)]} {
			return $config($opt)
		} else {
			return {}
		}
	}

	proc append {opt val} {
		variable config
		if {[info exists config($opt)]} {
			lappend config($opt) $val
		}
	}

	proc save {} {
		variable config
		# standard filename: ~/.esweep/impulse.conf
		# check for directory
		set dir [glob ~]/.esweep
		if {![file exists $dir]} {
			# create the config directory
			if {[catch {file mkdir $dir}]} {
				puts stderr "Can't create directory $dir"
				return 1
			}
		}
		# write all config options - except Intern - to the file
		set fp [open $dir/impulse.conf w]
		if {$fp == {}} {
			puts stderr {Can't open config file}
			return 1
		}
		foreach name [lsort -ascii [array names config -regexp {^((?!Intern).)*$}]] {
			if {$config($name) == {}} {
				set item {{}}
			} else {
				set item $config($name)
			}
			puts $fp "$name $item"
		}
		close $fp

		return 0
	}

	proc load {} {
		variable config
		# standard filename: ~/.esweep/impulse.conf
		# check for directory
		set filename [glob ~]/.esweep/impulse.conf
		if {[catch {set fp [open $filename r]}]} {
			return 1
		}
		if {$fp == {}} {
			puts stderr {Can't open config file}
			return 1
		}
		set conf [read $fp]
		close $fp
		# remove comments
		set conf [regsub -all {(?p)#.*} $conf { }]
		# remove newlines
		set conf [regsub -all {\n} $conf { }]
		# remove "bad" symbols
		set conf [regsub -all {[\[\]]} $conf { }]
		# apply as configuration
		array set config $conf

		return 0
	}

	namespace ensemble create -subcommands [list configure cget append save load]
}

##########################
# Principle of operation #
##########################

# The main window is tiled into 3 sub-windows: 
# * top left: frequency response
# * bottom left: diff window
# * right: list of overlays (quite small)
# Additional windows (diagrams) may be shown on toplevel (see command "show" below). 

# Operation is performed the vi-like way. In standard operation you are in command mode. 
# You navigate in the overlay window with the up/down or j/k keys. The current selected overlay is highlighted. 
# Then you have the following commands available: 
# * dd: delete selected overlay (it is copied in the standard buffer, see 'p')
# * yy: copy the selected overlay in the standard buffer
# * p/P: paste the content in the standard buffer after/before the selected overlay
# * ?id?m: measure the impulse response with mic id, e. g. 1m. 
#	   id is optional, and may be omitted (default: 1). 
#	   Up to 9 microphones may be used. If a microphone is not defined in the configuration, 
#	   an error is raised. 
#	   The result is put in the standard buffer (and displayed). 
# * ./u: redo/undo last operation (not possible for renaming overlays)
# * r: rename overlay (enters insertion mode). 
# * c: change the color of the selected overlay (a selection dialog appears)
# * <Ctrl-m>: Make the selected overlay to the master of the diff window. 
# All these commands are executed immediately. 
# Additional commands control the behaviour of the application. They are preceded by a : (colon): 
# * :cal ?id?: calibrate microphone id. If id is not specified, the default "1" is used. 
# * :set <option> ?value?: set configuration option "option" to "value"
#	If value is not specified, show the current value of "option"
# * :show ?window?: additional windows may be shown
# * :w ?filename?: write the overlays to filename. If filename is not specified, and no file was selected before, then a dialog window appears to select a file. 
# 		   If a file was selected before, it will be overwritten without further confirmation
# * :wq ?filename?: same as :w, but the application is exited. 
# * :q: exits the application. Does not exit when unsaved changes are pending. 
# * :q!: exits the application. All changes will be lost. 
# These operation commands are only executed after pressing "Enter". 

# This is the overlay array. There are three persistent buffers: Standard, Master and Undo. The Standard buffer is used to store 
# newly measured or copied/deleted overlays. The Master buffer holds the master curve of the diff window. 
# In the Undo buffer is a copy of the last changed overlay before the change. 
# Overlays are referenced by an ID. The ID is constantly up-counted during the runtime of the application (limited by View,MaxOverlays). 
# They consist of the sub-entries Name, Ir (the impulse response), Fr (the frequency response) and Diff (the difference to the master). 

namespace eval overlay {
	variable overlays
	array set overlays {
		Default,Name {Untitled}
		Default,Color green
		Default,IR {}
		Default,FR {}
		Default,Diff {}
		Default,Mic {}
		Default,Visibility {on}
		Default,Signal {}
		Default,SweepRate {}
		Temp,Name {}
		Standard,Name {}
		Undo,Name {}
		Master,Name {}
	}
	variable win .w.overlays
	variable lastID {}
	variable color {16 0 0}
	###############################
	# internal namespace commands #
	###############################
	
	proc __notAllowed {ID Filter} {
		if {[lsearch $Filter $ID]<0} {
			return 0
		} else { return 1}
	}

	proc __selectNext {{silent {}}} {
		variable win
		if {[set ID [$win next [__getSelectedID]]] != {}} {
			$win selection set $ID
			return $ID
		} elseif {$silent eq {}} {
			bell
		}
		return -1
	}

	proc __selectPrev {{silent {}}} {
		variable win
		if {[set ID [$win prev [__getSelectedID]]] != {}} {
			$win selection set $ID
			return $ID
		} elseif {$silent eq {}} {
			bell
		}
		return -1
	}

	proc __isSelected {} {
		variable win
		return [$win selection]
	}

	proc __createOverlayColor {} {
		variable color
		lassign $color r g b
		set c [expr {$r*256*256+$g*256+$b}]
		if {$c == 0} {
			set c 63
		} else {
			set c [expr {$c << 2}]
		}
		if {$c > 256*256*256+256*256+256} {
			set c 63
		}
		set b [expr {$c % 256}]
		set c [expr {$c / 256}]
		set g [expr {$c % 256}]
		set c [expr {$c / 256}]
		set r [expr {$c % 256}]
		::set color [list $r $g $b]
		return [format "#%02x%02x%02x" $r $g $b]
	}

	proc __deleteItemImage {ID} {
		variable win
		set img [$win item $ID -image]
		if {$img ne ""} {
			$win item $ID -image {}
			image delete $img
		}
	}

	proc __createItemImage {ID color} {
		variable win
		__deleteItemImage $ID
		set img [image create photo -height 10 -width 20]
		$img put $color -to 0 0 20 10 
		$win item $ID -image $img
	}

	proc __getSelectedID {} {
		variable win
		set item [$win selection]
		return [lindex $item 0]
	}

	# returns the index of the selected overlay, not the ID!
	proc __getSelectedIndex {} {
		variable win
		set index [$win index [__getSelectedID]]
		if {$index == ""} {
			return 0
		} else {
			return $index
		}
	}

	proc __updateName {ID entry} {
		variable win
		# string in entry
		if {[set name [$entry get]] ne ""} {
			# get old values
			set values [$win item $ID -values]
			lset values 1 $name
			configure $ID Name $name
			$win item $ID -values $values
		}
		destroy $entry
		impulse configure Intern,Unsaved 1
	}

	proc __rename {ID} {
		variable overlays
		variable win
		# ttk::treeview does not allow inplace editing
		# we place an entry just over the item
		lassign [$win bbox $ID 1] x y width height
		set name [entry $win.name -width $width]
		$name insert 0 [cget $ID Name]
		$name selection range 0 end
		place $name -x $x -y $y
		focus $name
		event add <<LeaveEntry>> <Return>
		event add <<LeaveEntry>> <FocusOut>
		bind $name <<LeaveEntry>> "::overlay::__updateName $ID $name; focus $win"
		bind $name <Escape> "destroy $name; focus $win"
	}

	##################################
	# namespace ensemble subcommands #
	##################################
	proc exists {ID {opt {}}} {
		variable overlays
		if {$opt eq {}} {set opt Name}
		if {[info exists overlays($ID,$opt)]} {
			return 1
		} else {
			return 0
		}
	}

	proc cget {ID {opt {}}} {
		variable overlays
		if {[exists $ID]} {
			if {$opt ne {}} {
				if {[info exists overlays($ID,$opt)]} {
					return $overlays($ID,$opt)
				} else {
					return {}
				}
			} else {
				# return only the option names
				# this avoids shimmering in some cases like: 
				# foreach {opt val} [cget $ID]
				# Use this instead: 
				# foreach {opt} [cget $ID] {
				# 	set val [cget $ID $opt]
				# 	...
				# }
				return [regsub -all $ID, [array names overlays $ID,*] {}]
			}
		}
		return {}
	}

	proc configure {ID opt {val {}} {noupdate {}}} {
		variable overlays
		# do not configure special and hidden IDs
		if {[__notAllowed $ID [list Undo Default]]} {return 1}
		if {![exists $ID]} {return 1}
		switch $opt {
			Color {
				if {$val == ""} {return 1}
				catch {__createItemImage $ID $val}
				# update display
			}
			Name {
				if {$val == ""} {
					if {![catch {expr {$ID+1}}]} {
						# update display
						__rename $ID
						return 0
					} else {
						return 1
					}
				}
			}
			Visibility {
				if {$val == {off}} {
					catch {__deleteItemImage $ID}
				} elseif {$val == {on}} {
					catch {__createItemImage $ID [cget $ID Color]}
				} else {return 1}
			}
			Mic {
				if {[catch {expr {int($val)}}]} {return 1}
			}
			FR {
				# do not display if $ID is a hidden buffer
				if {![__notAllowed $ID [list Temp Undo Default Master]]} {
					if {[::TkEsweepXYPlot::exists FR $ID]} {
						::TkEsweepXYPlot::configTrace FR $ID -trace $val
					} else {
						::TkEsweepXYPlot::addTrace FR $ID [cget $ID Name] $val
					}
				}
			}
			Diff {
				# do not display if $ID is a hidden buffer
				if {![__notAllowed $ID [list Temp Undo Default Master]]} {
					if {[::TkEsweepXYPlot::exists Diff $ID]} {
						::TkEsweepXYPlot::removeTrace Diff $ID
					}
					::TkEsweepXYPlot::addTrace Diff $ID [cget $ID Name] $val
				}
			}
			IR -
			default {
				# do nothing
				# this allows to simply add options which need not to
				# be handled somehow special
			}
		}
		set overlays($ID,$opt) $val
		# configure plot
		if {$noupdate eq {}} {
			if {[catch {::TkEsweepXYPlot::configTrace FR $ID -state $overlays($ID,Visibility) \
					-color $overlays($ID,Color) \
					-name $overlays($ID,Name)}]==0} {
				::TkEsweepXYPlot::plot FR 
			}
			if {[catch {::TkEsweepXYPlot::configTrace Diff $ID -state $overlays($ID,Visibility) \
					-color $overlays($ID,Color) \
					-name $overlays($ID,Name)}]==0} {
				::TkEsweepXYPlot::plot Diff
			}
		}
		return 0
	}

	proc delete {ID} {
		variable overlays
		variable win
		if {[__notAllowed $ID [list Temp Standard Undo Default Master]]} {return 1}
		if {[catch {array unset overlays $ID,*}]} {
			bell
			return 1
		} else {
			# remove overlay from listbox
			__deleteItemImage $ID
			$win delete $ID
			::TkEsweepXYPlot::removeTrace FR $ID
			catch {::TkEsweepXYPlot::removeTrace Diff $ID}
			::TkEsweepXYPlot::plot FR 
			::TkEsweepXYPlot::plot Diff
			return 0
		}
	}

	proc select {ID {silent {}}} {
		variable overlays
		variable win
		if {[__notAllowed $ID [list Temp Standard Undo Default Master]]} {return 1}
		if {$ID == {next}} {
			return [__selectNext $silent]
		} elseif {$ID == {prev}} {
			return [__selectPrev $silent]
		} elseif {[catch {$win selection set $ID}]} {
			if {$silent eq {}} {
				bell
			}
			return {}
		} else {
			return $ID
		}
	}

	proc copy {from to {noupdate {}}} {
		variable overlays
		if {[exists $from]} {
			catch {array unset overlays $to,*}
			set overlays($to,Name) [cget $from Name]
			foreach {opt} [cget $from] {
				configure $to $opt [cget $from $opt] $noupdate
			}
			return 0
		} else {
			bell
			return -1
		}
	}

	proc insert {} {
		variable overlays
		variable win
		variable lastID
		if {[llength [$win children {}]] < [impulse cget Intern,MaxOverlays] && [cget Standard Name] != ""} {
			# is this the first overlay? 
			if {$lastID == ""} {
				set lastID 0
			} else {
				incr lastID
			}
			set ID $lastID
			if {[copy Standard $ID noupdate] < 0} {return -1}
			configure $ID Name [cget Default Name] noupdate
			
			# insert the overlay in the listboxes
			$win insert {} [expr {[__getSelectedIndex]+1}] -id $ID -text $ID \
				-values [list \
				[cget $ID Mic] \
				[cget $ID Name]]
				
			# create the new color and image
			configure $ID Color  [__createOverlayColor]
			$win selection set $ID
			# set a new master when necessary
			if {[overlay master] == {}} {
				overlay master $ID
			}

			return $ID
		} else {
			bell
			return -1
		}
	}

	proc master {{ID {}}} {
		variable overlays
		variable win
		if {[__notAllowed $ID [list Temp Standard Undo Default Master]]} {return 1}
		if {$ID == {}} {
			return [$win tag has master]
		}

		if {$ID > -1} {
			set oldmaster [$win tag has master]
			# do nothing when oldmaster and ID are identical
			if {$oldmaster == $ID} return
			if {[overlay copy $ID Master noupdate]} return
			$win tag remove master $oldmaster
			$win item $ID -tags master
			$win tag add master $ID
		}
		return 0
	}

	proc traces {} {
		variable overlays
		variable win
		while {[__selectPrev silent] > -1} {continue}; # get the first available trace
		if {[set ID [lindex [__isSelected] 0]] == {}} return [list]
		lappend t $ID
		while {[set ID [__selectNext silent]] > -1} {
			lappend t $ID
		}
		return $t
	}

	namespace ensemble create \
		-subcommands [list exists cget configure delete select copy insert ID master traces] \
		-map [dict create ID __getSelectedID]
}
#########################
# end namespace overlay #
#########################

# Key bindings for overlay management
proc bindings {win} {
	bind $win <Double-d> [list deleteOverlay]
	bind $win <Double-y> [list yankOverlay]
	bind $win <p> [list pasteOverlay after]
	bind $win <P> [list pasteOverlay before]
	bind $win <period> [list redo]
	bind $win <u> [list undo]
	bind $win <r> [list renameOverlay]
	bind $win <c> [list changeColor]
	bind $win <v> [list toggleVisibility]
	bind $win <a> [list TkEsweepXYPlot::autoscale FR]

	bind $win <Control-m> [list makeMaster]

	# measurement bindings
	bind $win <m> [list measure]
	event add <<NumKey>> <Key-0>
	event add <<NumKey>> <Key-1>
	event add <<NumKey>> <Key-2>
	event add <<NumKey>> <Key-3>
	event add <<NumKey>> <Key-4>
	event add <<NumKey>> <Key-5>
	event add <<NumKey>> <Key-6>
	event add <<NumKey>> <Key-7>
	event add <<NumKey>> <Key-8>
	event add <<NumKey>> <Key-9>
	bind $win <<NumKey>> [list impulse append Intern,numBuffer %K]
	bind $win <Escape> [list impulse configure Intern,numBuffer {}]

	bind $win <Up> [list overlay select prev]
	bind $win <Down> [list overlay select next]

	# program control commands
	bind $win <colon> [list enterCommandLine]
}

# delete the selected overlay
proc deleteOverlay {} {
	if {[set n [join [impulse cget Intern,numBuffer] {}]] == {}} {set n 1}
	impulse configure Intern,numBuffer {}
	set ID [overlay ID]
	lappend deletedIDs $ID
	for {set i 1} {$i < $n} {incr i} {
		set ID [overlay select next silent]
		if {$ID == {}} break
		lappend deletedIDs $ID
	}
	if {[overlay select next silent] == {}} {
		# select the last item before the deleted range
		overlay select [lindex $deletedIDs 0]
		overlay select prev silent
	}
	foreach ID $deletedIDs {
		if {[overlay copy $ID Undo noupdate] < 0} return
		if {[overlay delete $ID] < 0} return
	}
	# set a new master when necessary
	if {[overlay master] == {}} {
		overlay master [overlay ID]
	}
	# rearrange display
	writeToCmdLine "deleted overlay ID: [join $deletedIDs]" -bg green
	impulse configure Intern,Unsaved 1
}

# yank the selected overlay to the Standard buffer
proc yankOverlay {} {
	set ID [overlay ID]
	if {[overlay copy $ID Standard noupdate] < 0} return
	overlay configure Standard Name Standard noupdate
	overlay configure Standard Color [overlay cget Default Color]
	writeToCmdLine "yanked overlay ID: $ID" -bg green
}

# paste the overlay from the standard buffer
proc pasteOverlay {position} {
	if {$position == {before}} {
		overlay select prev silent
	}
	if {[set ID [overlay insert]] < 0} return

	writeToCmdLine "pasted overlay with ID: $ID" -bg green
	impulse configure Intern,Unsaved 1
}

proc toggleVisibility {} {
	set ID [overlay ID]
	if {$ID > -1} {
		if {[overlay cget $ID Visibility] == {on}} {
			overlay configure $ID Visibility {off}
		} else {
			overlay configure $ID Visibility {on}
		}
		impulse configure Intern,Unsaved 1
	}
}

proc redo {} {
	puts stderr "not yet implemented"
}

proc undo {} {
	puts stderr "not yet implemented"
}

proc renameOverlay {} {
	set ID [overlay ID]
	overlay configure $ID Name
}

proc changeColor {} {
	set ID [overlay ID]
	if {[set newcolor [tk_chooseColor -initialcolor [overlay cget $ID Color] -title {Choose new color}]] ne {}} {
		overlay configure $ID Color $newcolor
	}
	impulse configure Intern,Unsaved 1
}

proc makeMaster {} {
	overlay master [overlay ID]
	if {[overlay master] != {}} {
		set master [overlay cget Master FR]
	} else {
		return 1
	}
	foreach ID [overlay traces] {
		if {[overlay master] == $ID} {
			catch {::TkEsweepXYPlot::removeTrace Diff $ID}
			continue
		}
		set fr [esweep::clone -src [overlay cget $ID FR]]
		esweep::expr fr - $master
		overlay configure $ID Diff $fr
	}
	return 0
	# recompute Diff
}

proc measure {} {
	overlay copy Default Temp noupdate
	# get the microphone number
	if {[set mic [join [impulse cget Intern,numBuffer] {}]] == {}} {set mic 1}
	writeToCmdLine "Measuring Mic $mic"
	impulse configure Intern,numBuffer {}
	overlay configure Temp Mic $mic

	# check the configuration, use defaults when necessary
	set out_channel [impulse cget Output,Channel]
	set in_channel [impulse cget Input,Channel]
	set ref_channel [impulse cget Input,Reference]
	if {$in_channel == $ref_channel} {
		bell
		writeToCmdLine {Input and reference channel are identical.} -bg red
		return
	}

	set max_channels [expr {$in_channel > $ref_channel ? $in_channel : $ref_channel}]
	set max_channels [expr {$max_channels > $out_channel ? $max_channels : $out_channel}]
	
	# get the configuration for the output channel
	# if not available, use global data
	if {[set outlevel [impulse cget Output,$out_channel,Level]] == {}} { 
		set outlevel [impulse cget Output,Global,Level]
	}
	if {[set outcal [impulse cget Output,$out_channel,Cal]] == {}} { 
		set outcal [impulse cget Output,Global,Cal]
	}
	if {[set outgain [impulse cget Output,$out_channel,Gain]] == {}} { 
		set outgain [impulse cget Output,Global,Gain]
	}
	set outmax [impulse cget Output,MaxFSLevel]

	set level [expr {$outlevel/($outgain*$outcal)}]
	if {$level < 0.0} {set level 0.0}
	if {$level > $outmax} {
		set level $outmax
		# use the opportunity und reset the output voltage for this channel if needed
		impulse configure Output,$out_channel,Level [set outlevel [expr {$level*$outgain*$outcal}]]
		writeToCmdLine "Adjusted the output level for channel $out_channel to $outlevel." -bg yellow
	}

	# we open the audio device now
	# because we need the framesize to avoid distortions
	set samplerate [impulse cget Audio,Samplerate]
	set dev "audio:[impulse cget Audio,Device]"
	set bitdepth [impulse cget Audio,Bitdepth]
	if {[catch {set au_hdl [esweep::audioOpen -device $dev]}]} {
			bell
			writeToCmdLine {Error opening audio device. Check configuration.} -bg red
			return 1
	}
	if {[esweep::audioConfigure -handle $au_hdl -param {precision} -value $bitdepth] < 0 || 
		[esweep::audioConfigure -handle $au_hdl -param {samplerate} -value $samplerate] < 0 || 
		    [esweep::audioConfigure -handle $au_hdl -param {channels} -value $max_channels] < 0} {
		bell
		writeToCmdLine {Error configuring audio device. Check configuration.} -bg red
		esweep::audioClose -handle $au_hdl
		return 1
	}

	set length [expr {int(0.5+([impulse cget Signal,Duration])*$samplerate/1000.0)}]
	# the length must be adjusted to the next integer multiple of the framesize
	set framesize [esweep::audioQuery -handle $au_hdl -param framesize]
	set length [expr {int(0.5+ceil(1.0*$length/$framesize)*$framesize)}]
	# the esweep audio system needs one signal for each channel
	# the unused channels will be zero
	set out [list]
	for {set i 0} {$i < $max_channels} {incr i} {
		lappend out [esweep::create -type wave -samplerate $samplerate -size $length]
	}
	
	set signal [lindex $out [expr {$out_channel-1}]]
	if {[set rate [::esweep::generate \
		[impulse cget Signal,Type] \
		-obj signal \
		-spectrum [impulse cget Signal,Spectrum] \
		-locut [impulse cget Signal,Locut] \
		-hicut [impulse cget Signal,Hicut] \
		]] < 0.0} {
			bell
			writeToCmdLine {Error generating signal. Check configuration.} -bg red
			return
	}
	overlay configure Temp Signal [impulse cget Signal,Type]
	overlay configure Temp SweepRate $rate
	esweep::expr signal * $level

	# The input signal must be long enough to get all of the system response
	# A delay of 200 ms will be enough for any delay
	set totlength [expr {int(0.5+$length+0.2*$samplerate)}]
	set totlength [expr {int(0.5+ceil(1.0*$totlength/$framesize)*$framesize)}]
	# the input signal containers
	set in [list]
	for {set i 0} {$i < $max_channels} {incr i} {
		lappend in [esweep::create -type wave -samplerate $samplerate -size $totlength]
	}

	# synchronize the in/out buffers
	esweep::audioSync -handle $au_hdl

	# measure
	set offset 0
	while {$offset < $length} {
		esweep::audioIn -handle $au_hdl -signals $in -offset $offset 
		set offset [esweep::audioOut -handle $au_hdl -signals $out -offset $offset]
	}
	while {$offset < $totlength} {
		set offset [esweep::audioIn -handle $au_hdl -signals $in -offset $offset] 
	}
	esweep::audioSync -handle $au_hdl

	# close the audio device
	esweep::audioClose -handle $au_hdl

	set in_system [lindex $in [expr {$in_channel-1}]]
	set in_ref [lindex $in [expr {$ref_channel-1}]]

	# calculate the IR
	esweep::deconvolve -signal in_system  -filter $in_ref
	overlay configure Temp IR $in_system 

	if {[__calcFr Temp]} {return}
	# calculate the Diff
	# if no Master is available, this isn't possible, so we don't check if it works or not
	__calcDiff Temp

	overlay configure Temp Name Standard noupdate
	overlay copy Temp Standard
}

proc __calcFr {ID} {
	set ir [overlay cget $ID IR]
	set mic [overlay cget $ID Mic]
	# the sensitivity is not necessarily available for each microphone
	# If not, then use the sensitivity that is stored for all mics 
	if {[set mic_sense [impulse cget Mic,$mic,Sensitivity]] == {}} {
		set mic_sense [impulse cget Mic,Global,Sensitivity]
	}
	set in_channel [impulse cget Input,Channel]
	set ref_channel [impulse cget Input,Reference]
	set out_channel [impulse cget Output,Channel]
	if {[set in_sense [impulse cget Input,$in_channel,Cal]] == {}} {
		set in_sense [impulse cget Input,Global,Cal]
	}
	if {[set ref_sense [impulse cget Input,$ref_channel,Cal]] == {}} {
		set ref_sense [impulse cget Input,Global,Cal]
	}
	if {[set outlevel [impulse cget Output,$out_channel,Level]] == {}} { 
		set outlevel [impulse cget Output,Global,Level]
	}

	if {[set dist [impulse cget Mic,$mic,Distance]] == {}} {
		# no distance stored, use the default
		set dist [impulse cget Mic,Global,Distance]
	}
	set samplerate [impulse cget Audio,Samplerate]
	if {$dist < 0} {
		# automatic gate adjustement
		set gate_start [expr {int(0.5+[esweep::maxPos -obj $ir]-0.002*$samplerate)}]
	} else {
		set gate_start [expr {int(0.5+$dist/343.0*$samplerate)}]
	}
	set gate_stop [expr {int(0.5+$gate_start+[impulse cget Processing,Gate]/1000.0*$samplerate)}]
	set gate_length [expr {$gate_stop - $gate_start+1}]
	set fft_length [impulse cget Processing,FFT,Size]
	if {$fft_length == 0} {
		set fft_length [expr {int(0.5+pow(2, int(0.5+ceil(log10($gate_length)/log10(2)))))}]
	} else {
		if {$fft_length < $gate_length} {
			bell
			writeToCmdLine {Gate length exceeds FFT length} -bg red
			return 1
		}
	}

	# the scaling factor
	set sc [expr {$in_sense/($ref_sense*$mic_sense)}]
	if {[impulse cget Processing,Scaling] eq {Absolute}} {
		set sc [expr {$sc*[impulse cget	Processing,Scaling,Reference]}]
	} elseif {[impulse cget Processing,Scaling] eq {Relative}} {
		set sc [expr {$sc*$outlevel}]
	} else {
		bell
		writeToCmdLine {Unknown scaling method.} -bg red
		return 1
	}

	# cut out the IR
	set fr [esweep::get -obj $ir -from $gate_start -to $gate_stop]
	esweep::toWave -obj fr
	esweep::fft -obj fr -inplace
	esweep::toPolar -obj fr
	esweep::expr fr * $sc
	esweep::lg -obj fr
	esweep::expr fr * 20
	if {[impulse cget Processing,Smoothing] > 0} {esweep::smooth -obj fr -factor [impulse cget Processing,Smoothing]}

	overlay configure $ID FR $fr
	return 0
}

proc __calcDiff {ID} {
	if {[overlay master] == $ID} {return 0}
	if {[overlay master] != {}} {
		set master [overlay cget Master FR]
		set fr [esweep::clone -src [overlay cget $ID FR]]
	} else {
		return 1
	}
	esweep::expr fr - $master
	overlay configure $ID Diff $fr
	return 0
}


proc enterCommandLine {} {
	.cmdLine configure -state normal
	.cmdLine delete 0 end
	.cmdLine insert 0 :
	focus .cmdLine
}

proc writeToCmdLine {txt args} {
	# do not allow enter the commandline while messaging
	bind .w.overlays <colon> {}
	.cmdLine configure -state normal
	foreach {opt value} $args {
		switch $opt {
			-fg {.cmdLine configure -fg $value}
			-bg {.cmdLine configure -readonlybackground $value}
			default {}
		}
	}
	.cmdLine delete 0 end
	.cmdLine insert 0 $txt
	.cmdLine configure -state readonly
	update
	# allow entering commandline after 1 second
	after 1000 .cmdLine configure -readonlybackground grey
	after 1000 bind .w.overlays <colon> [list enterCommandLine]
}

###########################
# The application windows #
###########################

###################
# The diff window #
###################

# During speaker development much of the time is spent by try-and-error, despite the
# fact that speaker simulation programs are getting better and better. In fact, every user 
# of a speaker simulation program uses try-end-error with the simulation, so apparently the only 
# advantage is that it might be faster (depending on the skill of the user). 

# This try-and-error time period may be vastly shortened by the use of a diff window. 
# This diff window displays the difference of the last N measured curves 
# (N may be configured) to a master (manually set). 
# The main benefit is, that you can see where exactly the changes appear, 
# and thus if they appear where you expected them and not where you not expected them. 
# Additionally, this diff window is perfect for directivity measurements. 
# Set the key bindings. For some bindings, additional proc's are necessary. They are implemented below. 

proc appWindows {} {
	####################
	# The command line #
	####################

	entry .cmdLine -state disabled -disabledforeground black
	bind .cmdLine <Return> {execCmdLine}
	bind .cmdLine <Escape> {.cmdLine configure -state disabled}


	#######################
	# The overlays window #
	#######################
	#
	frame .w

	# The overlays window consists of 4 listboxes in a frame: ID, Mic, Name, Color
	ttk::treeview .w.overlays -selectmode browse -columns [list Mic Name] 
	.w.overlays column #0 -minwidth 50 -width 50 -anchor w
	.w.overlays column Mic -minwidth 30 -width 30 -anchor w
	.w.overlays heading #0 -text {#ID} -anchor w
	.w.overlays heading Mic -text Mic
	.w.overlays heading Name -text Name
	.w.overlays tag configure master -background gold
	grab .w


	################
	# The diagrams #
	################

	frame .w.dia -bd 1px -background black
	set width [expr {[winfo width .]/2}]
	set height [expr {[winfo height .]/2}]
	canvas .w.dia.fr -bg white -width $width -height $height
	canvas .w.dia.diff -bg white -width $width -height $height
	pack .w.dia.fr .w.dia.diff -expand yes -fill both -side top

	pack .w.dia -side left -expand yes -fill both
	pack .w.overlays -side left -fill y
	pack .w -side top -expand yes -fill both
	pack .cmdLine -side bottom -fill x

	createPlots .w.dia.fr .w.dia.diff

	focus .w.overlays
}

proc createPlots {fr diff} {
	bind $fr <Configure> {::TkEsweepXYPlot::redrawCanvas FR}

	::TkEsweepXYPlot::init $fr FR
	set ::TkEsweepXYPlot::plots(FR,Config,Title) {Frequency response}

	set ::TkEsweepXYPlot::plots(FR,Config,Margin,Top) 10
	set ::TkEsweepXYPlot::plots(FR,Config,Margin,Bottom) 20
	set ::TkEsweepXYPlot::plots(FR,Config,MouseHoverDeleteDelay) 5000
	set ::TkEsweepXYPlot::plots(FR,Config,Legend,Align) center
	set ::TkEsweepXYPlot::plots(FR,Config,Cursors,1) off
	set ::TkEsweepXYPlot::plots(FR,Config,Cursors,2) off

	set ::TkEsweepXYPlot::plots(FR,Config,X,Label) {f [Hz]}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Precision) 5
	set ::TkEsweepXYPlot::plots(FR,Config,X,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Min) 20
	set ::TkEsweepXYPlot::plots(FR,Config,X,Max) 20000
	set ::TkEsweepXYPlot::plots(FR,Config,X,Bounds) [list 1 48000]
	set ::TkEsweepXYPlot::plots(FR,Config,X,Log) {yes}

	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Label) {SPL [dB]}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Precision) 3
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Autoscale) {on}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Min) -60
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Max) -10
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Bounds) [list -240 240]
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Log) {no}

	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Label) {Phase [rad]}
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Precision) 3
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Min) -3.14 
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Max) 3.14
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Log) {no}

	::TkEsweepXYPlot::plot FR

	bind $diff <Configure> {::TkEsweepXYPlot::redrawCanvas Diff}
	::TkEsweepXYPlot::init $diff Diff
	set ::TkEsweepXYPlot::plots(Diff,Config,Title) {Diff}
	set ::TkEsweepXYPlot::plots(Diff,Config,Margin,Top) 10
	set ::TkEsweepXYPlot::plots(Diff,Config,Margin,Bottom) 20
	set ::TkEsweepXYPlot::plots(Diff,Config,MouseHoverDeleteDelay) 5000
	set ::TkEsweepXYPlot::plots(Diff,Config,Legend,Align) center
	set ::TkEsweepXYPlot::plots(Diff,Config,Cursors,1) off
	set ::TkEsweepXYPlot::plots(Diff,Config,Cursors,2) off

	set ::TkEsweepXYPlot::plots(Diff,Config,X,Label) {f [Hz]}
	set ::TkEsweepXYPlot::plots(Diff,Config,X,Precision) 5
	set ::TkEsweepXYPlot::plots(Diff,Config,X,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(Diff,Config,X,Min) 20
	set ::TkEsweepXYPlot::plots(Diff,Config,X,Max) 20000
	set ::TkEsweepXYPlot::plots(Diff,Config,X,Bounds) [list 1 48000]
	set ::TkEsweepXYPlot::plots(Diff,Config,X,Log) {yes}

	set ::TkEsweepXYPlot::plots(Diff,Config,Y1,Label) {SPL [dB]}
	set ::TkEsweepXYPlot::plots(Diff,Config,Y1,Precision) 3
	set ::TkEsweepXYPlot::plots(Diff,Config,Y1,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(Diff,Config,Y1,Min) -6
	set ::TkEsweepXYPlot::plots(Diff,Config,Y1,Max) 6
	set ::TkEsweepXYPlot::plots(Diff,Config,Y1,Bounds) [list -240 240]
	set ::TkEsweepXYPlot::plots(Diff,Config,Y1,Log) {no}

	set ::TkEsweepXYPlot::plots(Diff,Config,Y2,Label) {Phase [rad]}
	set ::TkEsweepXYPlot::plots(Diff,Config,Y2,Precision) 3
	set ::TkEsweepXYPlot::plots(Diff,Config,Y2,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(Diff,Config,Y2,Min) -3.14 
	set ::TkEsweepXYPlot::plots(Diff,Config,Y2,Max) 3.14
	set ::TkEsweepXYPlot::plots(Diff,Config,Y2,Log) {no}

	::TkEsweepXYPlot::plot Diff
}


# Execute the command on the command line
proc execCmdLine {} {
	set cmd [.cmdLine get]
	focus .w.overlays
	# $cmd must be a valid command: it must start with a colon, but may
	# not contain sequences of 2 or more colons
	if {[regexp {{^(?!\:)|\:{2,}}} $cmd]} {
		bell
		writeToCmdLine "Invalid command: $cmd" -bg red
		after 1000 .cmdLine configure -state disabled -bg white -fg black
		return
	}
	if {[catch {eval $cmd}]} {
		#bell
		#writeToCmdLine "Command $cmd failed" -bg red
	}
	after 1000 .cmdLine configure -state disabled -bg white -fg black
}

#######################
# Command Line proc's #
#######################

proc :set {args} {
	if {[llength $args] > 1} {
		impulse configure [lindex $args 0] [lindex $args 1]
	} else {
		writeToCmdLine "[lindex $args 0]: [impulse cget [lindex $args 0]]"
	}
	return 0
}

proc :show {args} {
	writeToCmdLine {No other analyzing supported} -bg yellow
	return 0
}

proc :w {{filename ""}} {
	if {[impulse cget Intern,Filename] == ""} {
		if {$filename == ""} {
			if {[set filename [tk_getSaveFile]] == ""} {
				writeToCmdLine "Save cancelled" -bg yellow
				return 1
			}
		}	
	} else {
		set filename [impulse cget Intern,Filename]
	}
	# save file
	writeToCmdLine "Saved to $filename" -bg green
	impulse configure Intern,Unsaved 0
	impulse configure Intern,Filename $filename
	return 0
}

proc :wq {{filename ""}} {
	if {[:w $filename]} {
		writeToCmdLine "Couldn't save $filename. Use :wq! to override" -bg red
		return 1
	} else {
		exit
	}
}

proc :wq! {{filename ""}} {
	if {[:w $filename]} {return 1}
	exit
}

# exit, but check for unsaved changes
proc :q {} {
	if {[impulse cget Intern,Unsaved]} {
		writeToCmdLine "Unsaved changes. Use :q! to override" -bg red
		return 1
	}
	exit
}

# exit anyways
proc :q! {} {
	exit
}

proc :cal {{mic 1}} {
	puts "Calibrating microphone $mic ..."
}

###########
# Startup #
###########

# try to load config file
if {[impulse load]} {
	# not successful, write default config
	impulse save
}

appWindows
bindings .w.overlays
