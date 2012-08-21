#!/usr/local/bin/tclsh8.5
package require Tk 8.5
package require Ttk 8.5

load ../../esweep.so; # replace this line by something like package require ...
#load ../../esweep.dll
source ../TkEsweepXYPlot/TkEsweepXYPlot.tcl
source ./filterchain.tcl

############
# Overview #
############

# Load impulse responses for up to three channels, and create filters to crossover them. 
# See also the example/xover program. 

# This is the program configuration
namespace eval xover {
	variable config
	variable intern
	array set config {
		Channels,Out,Max 6
		Channels,In 1
		Audio,Samplerate {}
		Audio,Device {/dev/audio}
		Audio,Bitdepth 16
		Processing,FFTSize 32768
		Processing,Smoothing 6 
	}
	array set intern {
		Intern,numBuffer {}
		Intern,Filename {}
		Intern,Unsaved 0
		Intern,Play 0
	}
	# delete comments from array
	array unset config {#}

	proc configure {opt val} {
		variable config
		variable intern
		if {[lindex [split $opt ,] 0] == {Intern}} {
			if {[info exists intern($opt)]} {
				set intern($opt) $val
			} 
		} else {
			if {[info exists config($opt)]} {
				set config($opt) $val
			}
		}
	}

	proc cget {opt} {
		variable config
		variable intern
		if {[lindex [split $opt ,] 0] == {Intern}} {
			if {[info exists intern($opt)]} {
				return $intern($opt)
			} else {
				return {}
			}
		} else {
			if {[info exists config($opt)]} {
				return $config($opt)
			} else {
				return {}
			}
		}
	}

	proc append {opt val} {
		variable config
		variable intern
		if {[lindex [split $opt ,] 0] == {Intern}} {
			if {[info exists intern($opt)]} {
				lappend intern($opt) $val
			}
		} else {
			if {[info exists config($opt)]} {
				lappend config($opt) $val
			}
		}
	}


	namespace ensemble create -subcommands [list configure cget append]
}

# This is the impulse responses manager
namespace eval channels {
	variable config
	array set config {
	}
	variable sum
	array set sum {
	}
	# Standard filter coefficients
	# These coefficients were taken from
	# Tietze/Schenk, "Halbleiterschaltungstechnik", 2010
	variable coeffs
	array set coeffs {
		Critical {
			1 {-} 2 {0.5} 3 {- 0.5} 4 {0.5 0.5} 5 {- 0.5 0.5} 6 {0.5 0.5 0.5} 
			7 {- 0.5 0.5 0.5} 8 {0.5 0.5 0.5 0.5} 9 {- 0.5 0.5 0.5 0.5}
			10 {0.5 0.5 0.5 0.5 0.5}
		}
		Bessel {
			1 {-} 2 {0.58} 3 {- 0.69} 4 {0.52 0.81} 5 {- 0.56 0.92} 6 {0.51 0.61 1.02}
			7 {- 0.53 0.66 1.13} 8 {0.51 0.56 0.71 1.23} 9 {- 0.52 0.59 0.76 1.32}
			10 {0.5 0.54 0.62 0.81 1.42}
		}
		Butterworth {
			1 {-} 2 {0.71} 3 {- 1.0} 4 {0.54 1.31} 5 {- 0.62 1.62} 6 {0.52 0.71 1.93}
			7 {- 0.55 0.8 2.25} 8 {0.51 0.60 0.90 2.56} 9 {- 0.53 0.65 1.0 2.88}
			10 {0.51 0.56 0.71 1.1 3.2}
		}
		Chebychev1dB {
			1 {-} 2 {0.86} 3 {- 1.71} 4 {0.71 2.94} 5 {- 1.18 4.54} 
			6 {0.68 1.81 6.51} 7 {- 1.09 2.58 8.84} 8 {0.68 1.61 3.47 11.53}
			9 {- 1.06 2.21 4.48 14.58} 10 {0.67 1.53 2.89 5.61 17.99}
		}
		Chebychev3dB {
			1 {-} 2 {0.96} 3 {- 2.02} 4 {0.78 3.56} 5 {- 1.4 5.56} 6 {0.76 2.2 8.0} 
			7 {- 1.3 3.16 10.9} 8 {0.75 1.96 4.27 14.24} 9 {- 1.26 2.71 5.53 18.03}
			10 {0.75 1.86 3.56 6.94 22.26}
		}
	}

	#####################
	# Internal commands #
	# ###################
	#
	proc cget {opt} {
		variable config
		if {[info exists config($opt)]} {
			return $config($opt)
		} else {
			return {}
		}
	}

	proc configure {opt val} {
		variable config
		set config($opt) $val
	}

	proc load {} {
		variable config

		if {[llength [::filterchain::getChannels]] <= 0} {bell; return -code ok}

		set w [toplevel .[clock seconds] -class TkDialog]
		wm resizable $w 0 0
		wm title $w {Load IR}

		grab $w

		bind $w <Escape> "set ${w}_file {}; set done 0"
		bind $w <Return> {set done 1}
		bind $w <KP_Enter> {set done 1}

		label  $w.lchannel -text {Channel}
		::ttk::spinbox $w.channel -textvar ${w}_channel -state readonly \
				-wrap yes -values [::filterchain::getChannels]
		set ::${w}_channel [lindex [::filterchain::getChannels] 0]

		label $w.ldirac -text {Create Dirac}
		variable ::${w}_dirac
		::ttk::checkbutton $w.dirac -variable ${w}_dirac -command [list ::channels::toggleFileSelect $w.filebtn ${w}_dirac]
		set ::${w}_dirac 0

		label $w.lfile -text {IR file:} 
		::ttk::entry $w.file -state readonly -textvar ${w}_file
		set ::${w}_file {}
		
		::ttk::button $w.filebtn -text Browse -command [list ::channels::getIRFilename $w.file]

		grid $w.lchannel  -    $w.channel  -sticky news
		grid $w.ldirac  -    $w.dirac  -sticky news
		grid $w.file  -    $w.file $w.filebtn -sticky news

		button $w.ok     -text OK     -command {set done 1}
		button $w.cancel -text Cancel -command "set ${w}_file {}; set done 0"
		grid $w.ok $w.cancel
		focus $w.channel
		vwait done
		grab release $w
		destroy $w

		set channel [set ::${w}_channel]
		set dirac [set ::${w}_dirac]
		set filename [set ::${w}_file]

		if {$dirac == 0 && $filename eq {}} {return}

		if {$dirac} {
			# create a dummy Dirac impulse
			if {[::xover cget Audio,Samplerate] == {}} {::xover configure Audio,Samplerate 48000}
			set ir [esweep::create -type {wave} -samplerate [::xover cget Audio,Samplerate] -size [::xover cget Processing,FFTSize]]
			esweep::generate dirac -obj ir
		} else {
			if {[catch {set ir [esweep::load -filename $filename]} errInfo]} {
				return -code error $errInfo
			}
			if {[esweep::type -obj $ir] != {wave}} {
				return -code error {No valid impulse response!}
			}
			if {[::xover cget Audio,Samplerate] == {}} {
				::xover configure Audio,Samplerate [esweep::samplerate -obj $ir]
			} else {
				if {[::xover cget Audio,Samplerate] != [esweep::samplerate -obj $ir]} {
					return -code error {Loaded impulse response with wrong samplerate}
				}
			}
		}
		set config($channel,IR) $ir
		set config($channel,FR) [__calcFR $ir]

		displayChannel $channel
	}

	proc toggleFileSelect {w var} {
		global $var
		if {[set $var] == 1} {
			$w configure -state disabled
		} else {
			$w configure -state normal
		}
	}

	proc getIRFilename {w} {
		set types {
		    {{IR Files}       {.ir}        }
		    {{All Files}        *             }
		}
		set filename [tk_getOpenFile -filetypes $types]

		if {$filename != ""} {
			$w configure -state normal
			$w delete 0 end
			$w insert 0 $filename 
			$w configure -state readonly
		}
	}

	proc __calcFR {response} {
		set fr [esweep::fft -obj response]
		esweep::toPolar -obj fr
		if {[::xover cget Processing,Smoothing] > 0} {esweep::smooth -obj fr -factor [::xover cget Processing,Smoothing]}
		esweep::lg -obj fr
		esweep::expr fr * 20
		return $fr
	}

	proc getUnmappedChannels {} {
		variable config
		set mapped [list]
		foreach {name channel} [array get config *,Map] {
			lappend mapped $channel
		}
		set unmapped [list]
		for {set m 1} {$m <= [::xover cget Channels,Out,Max]} {incr m} {
			if {[lsearch $mapped $m] < 0} {
				lappend unmapped $m
			}
		}
		return $unmapped
	}

	proc getMappings {} {
		variable config
		set mapped [list]
		foreach {name channel} [array get config *,Map] {
			lappend mapped [lrange [split $name ,] 0 end-1] $channel
		}
		return $mapped
	}

	proc create {} {
		variable config
		# create a new channel
		# give the window a unique name
		set w [toplevel .[clock seconds] -class TkDialog]
		wm resizable $w 0 0
		wm title $w {Create channel}

		grab $w

		bind $w <Escape> "set ${w}_name {}; set done 1" 
		bind $w <Return> {set done 1}
		bind $w <KP_Enter> {set done 1}

		label  $w.lname -text {Channel name}
		entry  $w.name -textvar ${w}_name -bg white

		label $w.lmap -text {Map}
		spinbox $w.map -values [getUnmappedChannels] -wrap yes \
				-textvar ${w}_map -state readonly

		button $w.ok     -text OK     -command {set done 1}
		button $w.cancel -text Cancel -command "set ${w}_name {}; set done 1"
		grid $w.lname  -    $w.name  -sticky news
		grid $w.lmap  -    $w.map  -sticky news
		grid $w.ok $w.cancel
		focus $w.name
		vwait done
		grab release $w
		destroy $w

		set name [set ::${w}_name]
		set map [set ::${w}_map]
		if {$name eq {}} {return -code ok}

		::filterchain::addChannel $name $map
		::filterchain::drawChannel $name

		set config($name,Map) $map
	}

	proc addFilter {} {
		variable config
		variable coeffs
		if {[llength [::filterchain::getChannels]] <= 0} {bell; return -code ok}
		# create a new channel
		# give the window a unique name
		set w [toplevel .[clock seconds] -class TkDialog]
		wm resizable $w 0 0
		wm title $w {Create filter}

		grab $w

		bind $w <Escape> "set ${w}_channel {}; set done 1" 
		bind $w <Return> {set done 1}
		bind $w <KP_Enter> {set done 1}

		label  $w.lchannel -text {Channel}
		::ttk::spinbox $w.channel -textvar ${w}_channel -state readonly \
				-wrap yes -values [::filterchain::getChannels]
		set ::${w}_channel [lindex [::filterchain::getChannels] 0]

		label $w.ltype -text {Type}
		::ttk::spinbox $w.type -textvar ${w}_type -state readonly \
				-wrap yes -values [list Lowpass Highpass Bandpass Bandstop Generic]
		set ::${w}_type Lowpass

		label $w.lapprox -text {Approximation}
		::ttk::spinbox $w.approx -textvar ${w}_approx -state readonly -wrap yes \
			-values [list Critical Bessel Butterworth Chebychev1dB Chebychev3dB]
		set ::${w}_approx Butterworth

		label $w.lorder -text {Order}
		::ttk::spinbox $w.order -textvar ${w}_order -state readonly -wrap yes \
			-from 1 -to 10 -increment 1
		set ::${w}_order 2

		label $w.lfreq -text Frequency
		ttk::entry $w.freq -textvar ${w}_freq -validate key \
				-validatecommand [list ::filterchain::valNumber %d %S %P {> 0}]
		set ::${w}_freq 1000

		button $w.ok     -text OK     -command {set done 1}
		button $w.cancel -text Cancel -command "set ${w}_channel {}; set done 1"

		grid $w.lchannel  -    $w.channel  -sticky news
		grid $w.ltype  -    $w.type  -sticky news
		grid $w.lapprox  -    $w.approx  -sticky news
		grid $w.lorder  -    $w.order  -sticky news
		grid $w.lfreq  -    $w.freq  -sticky news

		grid $w.ok $w.cancel
		focus $w.channel
		vwait done
		grab release $w
		destroy $w

		set channel [set ::${w}_channel]
		set type [set ::${w}_type]
		set approx [set ::${w}_approx]
		set order [set ::${w}_order]
		set freq [set ::${w}_freq]
		if {$channel eq {}} {return -code ok}

		if {$type eq {Generic}} {
			# Standard biquad filter, gain=1
			::filterchain::addFilter $channel {gain} 1 1 1 1 1
			::filterchain::drawChannel $channel
			return -code ok
		}

		# all other filters are created from multiple biquads
		# and then grouped together
		set ids [list]
		array set coeff $coeffs($approx)
		foreach c $coeff($order) {
			if {$c eq {-}} {
				# single order filter; this only happens when the filter is odd order
				# Bandpass and Bandstop are not possible with odd order, so bail out
				# when this type is the filter
				if {$type ne {Lowpass} && $type ne {Highpass}} {
					return -code error "Filter type $type not possible with odd order!"
				}
				# we must use integrators or differentiators
				if {$type eq {Lowpass}} {
					set t {integrator}
				} elseif {$type eq {Highpass}} {
					set t {differentiator}
				} else {
					set t [string tolower $type]
				}
				lappend ids [::filterchain::addFilter \
						$channel $t \
						1 1 $freq 1 1]
			} else {
				lappend ids [::filterchain::addFilter $channel \
						[string tolower $type] \
						1 $c $freq 1 1]
			}
		}
		# group the filters together when more than 1
		if {[llength $ids] > 1} {
			::filterchain::createGroup $channel "$type\n$approx\nN=$order\nF=$freq Hz" $ids
		}
		::filterchain::drawChannel $channel
		displayChannel $channel
		return -code ok
	}

	proc getFilter {channel} {
		set filterchain [::filterchain::getFilters $channel]
		if {[llength $filterchain] > 0} {
			set f [lindex $filterchain 0]
			lassign $f type gain Qp Fp Qz Fz
			set filter [::esweep::createFilterFromCoeff -type $type -gain $gain -Qp $Qp -Fp $Fp -Qz $Qz -Fz $Fz -samplerate [::xover cget Audio,Samplerate]]

			for {set i 1} {$i < [llength $filterchain]} {incr i} {
				set f [lindex $filterchain $i]
				lassign $f type gain Qp Fp Qz Fz
				set filter [::esweep::appendFilter -filter $filter \
				 -append [::esweep::createFilterFromCoeff -type $type -gain $gain -Qp $Qp -Fp $Fp -Qz $Qz -Fz $Fz \
				 -samplerate [::xover cget Audio,Samplerate]]]
			 }
			 return $filter
		} else {
			return {}
		}
	 }

	proc displayChannel {channels {f {}}} {
		variable config
		variable sum

		# filter the IR
		foreach ch $channels {
			if {![info exists config($ch,IR)]} {continue}
			set config($ch,FilteredIR) [::esweep::clone -src $config($ch,IR)]	
			if {[llength [set filter [getFilter $ch]]] > 0} {
				::esweep::filter -signal config($ch,FilteredIR) -filter $filter
			}

			# display the channel
			set config($ch,FR) [__calcFR $config($ch,FilteredIR)]
			if {[::TkEsweepXYPlot::exists FR [::filterchain::getMap $ch]]} {
				::TkEsweepXYPlot::configTrace FR [::filterchain::getMap $ch] -trace $config($ch,FR)
			} else {
				::TkEsweepXYPlot::addTrace FR [::filterchain::getMap $ch] $ch $config($ch,FR)
			}
		}

		# recalculate the sum
		if {[llength [::filterchain::getChannels]] > 1} {
			# find the longest IR; this must be our master
			foreach ch [::filterchain::getChannels] {break}
			set max_size [::esweep::size -obj $config($ch,FilteredIR)]
			set max_ch $ch
			foreach ch [::filterchain::getChannels] {
				if {![info exists config($ch,FilteredIR)]} continue
				set size [::esweep::size -obj $config($ch,FilteredIR)]]
				if {$size > $max_size} {
					set max_size $size
					set max_ch $ch
				}
			}

			set sum(IR) [::esweep::clone -src $config($max_ch,FilteredIR)]
			foreach ch [::filterchain::getChannels] {
				if {![info exists config($ch,FilteredIR)]} continue
				if {$ch eq $max_ch} {continue}
				::esweep::expr sum(IR) + $config($ch,FilteredIR)
			}
			set sum(FR) [__calcFR $sum(IR)]
			catch {::TkEsweepXYPlot::removeTrace FR $sum(ID)}
			set sum(ID) [::TkEsweepXYPlot::addTrace FR [clock microseconds] {Sum} $sum(FR)]
			::TkEsweepXYPlot::configTrace FR $sum(ID) -color darkgreen
		}
		::TkEsweepXYPlot::plot FR
	}

	namespace ensemble create -subcommands [list create load cget configure getMappings]
	namespace ensemble create -command ::filter -map [dict create add addFilter get getFilter]
}

# Key bindings 
proc bindings {win} {
	bind $win <c> [list ::channels create]
	bind $win <l> [list ::channels load]
	bind $win <a> [list ::filter add]
	bind $win <p> [list :play]
	event add <<NumKey>> <Key-0>
	event add <<NumKey>> <KP_0>
	event add <<NumKey>> <Key-1>
	event add <<NumKey>> <KP_1>
	event add <<NumKey>> <Key-2>
	event add <<NumKey>> <KP_2>
	event add <<NumKey>> <Key-3>
	event add <<NumKey>> <KP_3>
	event add <<NumKey>> <Key-4>
	event add <<NumKey>> <KP_4>
	event add <<NumKey>> <Key-5>
	event add <<NumKey>> <KP_5>
	event add <<NumKey>> <Key-6>
	event add <<NumKey>> <KP_6>
	event add <<NumKey>> <Key-7>
	event add <<NumKey>> <KP_7>
	event add <<NumKey>> <Key-8>
	event add <<NumKey>> <KP_8>
	event add <<NumKey>> <Key-9>
	event add <<NumKey>> <KP_9>
	bind $win <<NumKey>> [list xover append Intern,numBuffer %K]
	bind $win <Escape> [list xover configure Intern,numBuffer {}]

	# bindings for the filterchain
	bind $win <g> [list ::filterchain::groupSelected]
	bind $win <x> [list ::filterchain::expandSelected]
	bind $win <e> [list ::filterchain::editSelected]
	bind $win <Delete> [list ::filterchain::deleteSelected]
	bind $win <s> [list ::filterchain::toggleSuppress]

	# program control commands
	bind $win <colon> [list enterCommandLine]
}

proc deleteBindings {win {nobell 0}} {
	set binds [bind $win]
	foreach seq $binds {
		bind $win $seq [expr {$nobell ne {} ? {} : {bell}}]
	}
	return $binds
}

proc enterCommandLine {} {
	bind .cmdLine <Return> {execCmdLine}
	bind .cmdLine <Escape> [list exitCmdLine]
	.cmdLine configure -state normal
	.cmdLine delete 0 end
	.cmdLine insert 0 :
	deleteBindings . nobell
	focus -force .cmdLine
	grab .cmdLine
	update idletasks
}

proc exitCmdLine {{delay 0}} {
	after $delay .cmdLine configure -state disabled -bg white -fg black
	grab release .cmdLine
	focus -force . 
	bindings .
	update idletasks
}

proc writeToCmdLine {txt args} {
	# do not allow enter the commandline while messaging
	bind . <colon> {}
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
	after 1000 bind . <colon> [list enterCommandLine]
}

###########################
# The application windows #
###########################

proc appWindows {} {
	####################
	# The command line #
	####################

	entry .cmdLine -state disabled -disabledforeground black

	################
	# The diagrams #
	################

	frame .dia -bd 1px -background black -class XOver

	frame .chain -bd 1px -background black -class XOver

	pack .dia .chain -side top -expand yes -fill both
	pack .cmdLine -side bottom -fill x

	createPlots .dia 
	::filterchain::init .chain ::channels::displayChannel
	focus .
}

proc createPlots {fr} {
	::TkEsweepXYPlot::init $fr FR
	bind $fr <Configure> {::TkEsweepXYPlot::redrawCanvas FR}

	set ::TkEsweepXYPlot::plots(FR,Config,Title) {Frequency response}

	set ::TkEsweepXYPlot::plots(FR,Config,Margin,Top) 10
	set ::TkEsweepXYPlot::plots(FR,Config,Margin,Bottom) 20
	set ::TkEsweepXYPlot::plots(FR,Config,MouseHoverDeleteDelay) 5000
	set ::TkEsweepXYPlot::plots(FR,Config,Legend,Align) center

	set ::TkEsweepXYPlot::plots(FR,Config,X,Label) {f [Hz]}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Precision) 5
	set ::TkEsweepXYPlot::plots(FR,Config,X,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,X,Min) 20
	set ::TkEsweepXYPlot::plots(FR,Config,X,Max) 20000
	set ::TkEsweepXYPlot::plots(FR,Config,X,Log) {yes}

	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Label) {SPL [dB]}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Precision) 3
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Min) -40
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Max) 10
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Bounds) [list -240 240]
	set ::TkEsweepXYPlot::plots(FR,Config,Y1,Log) {no}

	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Label) {Phase [rad]}
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Precision) 3
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Autoscale) {off}
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Min) -3.14 
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Max) 3.14
	set ::TkEsweepXYPlot::plots(FR,Config,Y2,Log) {no}

	::TkEsweepXYPlot::plot FR
}


# Execute the command on the command line
proc execCmdLine {} {
	set cmd [.cmdLine get]
	# $cmd must be a valid command: it must start with a colon, but may
	# not contain sequences of 2 or more colons
	if {[regexp {{^(?!\:)|\:{2,}}} $cmd]} {
		bell
		writeToCmdLine "Invalid command: $cmd" -bg red
		after 1000 .cmdLine configure -state disabled -bg white -fg black
		return
	}
	#eval $cmd
	if {[catch {eval $cmd} result]} {
		bell
		writeToCmdLine "Command $cmd failed  with: $result" -bg red
	}
	exitCmdLine 1000
}

#######################
# Command Line proc's #
#######################

proc :set {args} {
	if {[llength $args] > 1} {
		::xover configure [lindex $args 0] [lindex $args 1]
	} else {
		writeToCmdLine "[lindex $args 0]: [::xover cget [lindex $args 0]]"
	}
	return 0
}

proc :e {{filename ""}} {
	if {[::xover cget Intern,Unsaved]} {
		# we need to save the old file first
		set result [tk_dialog .filesavedialog -title {Unsaved changes} \
			-text "Save changes to [::xover cget Intern,Filename]?" \
			-default 0 \
			{Yes} {No} {Cancel}]
		switch $result {
			Yes {
				:w [::xover cget Intern,Filename]
			}
			No {
				# do nothing
			}
			Cancel {
				# bail out
				return 0
			}
		}
	}
	if {$filename == ""} {
		if {[set filename [tk_getOpenFile]] == ""} {
			writeToCmdLine "Load cancelled" -bg yellow
			return 0
		}
	}	
	# load file
	::filterchain::loadChain $filename

	# get the channel information
	foreach ch [::filterchain::getChannels] {
		channels configure $ch,Map [::filterchain::getMap $ch]
	}
	
	writeToCmdLine "Loaded $filename" -bg green

	::xover configure Intern,Unsaved 0
	::xover configure Intern,Filename $filename
	return 0
}

proc :w {{filename ""}} {
	if {[::xover cget Intern,Filename] == ""} {
		if {$filename == ""} {
			if {[set filename [tk_getSaveFile]] == ""} {
				writeToCmdLine "Save cancelled" -bg yellow
				return 1
			}
		}	
	} else {
		set filename [::xover cget Intern,Filename]
	}
	# save file
	::filterchain::saveChain $filename
	
	writeToCmdLine "Saved to $filename" -bg green

	::xover configure Intern,Unsaved 0
	::xover configure Intern,Filename $filename
	return 0
}

proc :wq {{filename ""}} {
	if {[:w $filename]} {
		writeToCmdLine "Couldn't save $filename. Use :wq! to override" -bg red
		return 0
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
	if {[::xover cget Intern,Unsaved]} {
		writeToCmdLine "Unsaved changes. Use :q! to override" -bg red
		return 1
	}
	exit
}

# exit anyways
proc :q! {} {
	exit
}

proc openAudioDevice {{out_channels {1}} {in_channels {1}}} {
	set samplerate [::xover cget Audio,Samplerate]
	set dev [::xover cget Audio,Device]
	set bitdepth [::xover cget Audio,Bitdepth]
	if {[catch {set au_hdl [esweep::audioOpen -device $dev -samplerate $samplerate]}]} {
			bell
			writeToCmdLine {Error opening audio device. Check configuration.} -bg red
			return -code error {Error opening audio device. Check configuration.}
	}
	if {[esweep::audioConfigure -handle $au_hdl -param {precision} -value $bitdepth] < 0} {
		bell
		esweep::audioClose -handle $au_hdl
		return -code error {Error configuring bitdepth on audio device. Check configuration.} 
	}
	# check the number of channels; if there are not enough to use the output channel, 
	# try to configure some more
	if {$out_channels > [esweep::audioQuery -handle $au_hdl -param {play_channels}]} {
		 if {[esweep::audioConfigure -handle $au_hdl -param {play_channels} -value $out_channel] < 0} {
			bell
			esweep::audioClose -handle $au_hdl
			return -code error {Error configuring playback channel. Check configuration.} 
		}
	}
	if {$in_channels > [esweep::audioQuery -handle $au_hdl -param {record_channels}]} {
		 if {[esweep::audioConfigure -handle $au_hdl -param {record_channels} -value $in_channel] < 0} {
			bell
			esweep::audioClose -handle $au_hdl
			return -code error {Error configuring record channel. Check configuration.} 
		}
	}
		
	return $au_hdl
}

proc :play {} {
	set samplerate [::xover cget Audio,Samplerate]

	# find the highest output channel
	# we need to open the soundcard with at least this
	# number of channels
	set max_channels 0
	set used_channels [list]
	# create filter for each needed output channels
	array set mapping {}
	foreach {name map} [::channels getMappings] {
		if {$map > $max_channels} {
			set max_channels $map
		}
		lappend used_channels $map
		set mapping($map,Filter) [::filter get $name]
	}
	# create dummy filters for the unneeded channels
	for {set i 1} {$i < $max_channels} {incr i} {
		if {[lsearch $used_channels $i] < 0} {
			set mapping($i,Filter) [::esweep::createFilterFromCoeff -type gain -samplerate $samplerate -gain 1]
		}
	}

	set au [openAudioDevice $max_channels [::xover cget Channels,Input]]
	set framesize [esweep::audioQuery -handle $au -param framesize]

	set in [list]
	set out [list]
	for {set i 0} {$i < $max_channels} {incr i} {
		lappend out [esweep::create -type wave -samplerate $samplerate -size $framesize]
		set mapping([expr {$i+1}],Signal) [lindex $out end]
	}
	for {set i 0} {$i < [::xover cget Channels,In]} {incr i} {
		lappend in [esweep::create -type wave -samplerate $samplerate -size $framesize]
	}
	set in_channel [lindex $in end]

	# play
	deleteBindings . 
	bind . <Escape> [list ::xover configure Intern,Play 0]
	# synchronize the in/out buffers
	esweep::audioSync -handle $au
	::xover configure Intern,Play 1
	puts {Starting playback...}
	while {[::xover cget Intern,Play]} {
		esweep::audioIn -handle $au -signals $in 
		for {set i 1} {$i <= $max_channels} {incr i} {
			esweep::copy -dst mapping($i,Signal) -src $in_channel
			esweep::filter -signal mapping($i,Signal) -filter $mapping($i,Filter)
			if {[esweep::max -obj $mapping($i,Signal)] > 1 || [esweep::min -obj $mapping($i,Signal)] < -1} {
				puts stderr "Channel $i: Overload"
			}
		}
		esweep::audioOut -handle $au -signals $out
		update
	}
	puts {Stopping playback...}
	bindings .
	
	# close the audio device
	esweep::audioSync -handle $au
	esweep::audioClose -handle $au
}

###########
# Startup #
###########

appWindows
bindings . 
#console show
