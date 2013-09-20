# Configuration module of both impulse applications

namespace eval ::impulse::config {
	variable config
	# default configuration
	array set config {
		# {Debugging}
		Debug			1
		# {File where the configuration is stored}
		Filename 		{}
		# {Audio device}
		Audio,Device 		{audio:/dev/audio0}
		# {Samplerate}
		Audio,Samplerate 	48000
		# {bitrate}
		Audio,Bitdepth 		16
		# {Microphone sensitivity in V/Pa}
		Mic,Global,Sensitivity 	1
		# {Microphone frequency response compensation file \
		   If empty, no compensation is done}
		Mic,Global,Compensation ""
		# {Microphone distance, will be removed from the impulse response} 
		# {for maximum frequency resolution.}
		# {If the distance is < 0, then the program searches for the highest peak}
		# {If you measure systems with very low upper corner frequency (e. g. subwoofers)}
		# {you should not use the automatic mode}
		Mic,Global,Distance 	-1
		# {output channel}
		Output,Channel		1 
		# {output level in Vrms}
		Output,Global,Level	1.0 
		# {gain of external amplifier}
		Output,Global,Gain 	1.0
		# {Maximum output level in FS. If < 1 avoids oversteering}
		Output,MaxFSLevel 	0.9 
		# {Calibration level for the output in Vpeak/FS}
		Output,Global,Cal	1.0 
		# {Signal type, only logweep allowed at the moment}
		Signal,Type 		logsweep
		# {Signal duration in ms}
		Signal,Duration 	2000
		# {Signal spectrum, allowed values: white, pink, red}
		# {pink is the most useful in acoustic applications}
		Signal,Spectrum 	pink
		# {Signal low cutoff in Hz}
		# {Actually, there are significant spectral components above and below both cutoffs}
		# {The lower cutoff means that the energy below and above is reduced}
		Signal,Locut 		20
		# {Signal high cutoff in Hz}
		Signal,Hicut 		20000
		# {Main input channel} 
		Input,Channel		1
		# {Reference channel}
		Input,Reference 	2
		# {gain of external amplifier}
		Input,Global,Gain 	1.0
		Input,1,Gain		1.0
		Input,2,Gain 		1.0
		# {Calibration level for the input in FS/Vpeak}
		Input,Global,Cal 	1.0
		# {Single or dual channel}
		Processing,Mode 	Single
		# {Length of gate in ms}
		Processing,Gate 	100
		# {Length of FFT in samples}
		# {if 0 then the length is automatically selected}
		Processing,FFT,Size 	0
		# {The scaling can be either Relative (SPL/Pa/V)}
		# {or Absolute (SPL). In dual channel mode, only relative scaling is allowed.}
		Processing,Scaling 	Relative
		# {The reference level}
		Processing,Scaling,Reference 2.83
		# {Smoothing factor}
		Processing,Smoothing 	6
		# {Range of FR display}
		Processing,FR,Range 	50
		# {Show which distortions? The number is the n'th harmonic}
		Processing,Distortions 	{2 3 5 7}
		# {Range of FR display with distortions}
		Processing,Distortions,Range 80
		# {Shift K1 by a specified amount to visualize a limit}
		Processing,Distortions,K1Shift -40
    # {Distortion weighting: None: no weighting; Linear: each component is weighted by n/2, Quadratic: weighted by n^2/4} 
		Processing,Distortions,Weighting {Quadratic}

		# {Internal part}
		# {hold the typed numbers for e. g. 12dd}
		Intern,numBuffer {}
		# {needed for blocking of a few functions while measuring}
		Intern,Play 0
		# {remember the last used mic; needed for live update}
		Intern,LastMic 1
		# {Hold an event ID to make live update smooth}
		Intern,LiveRefresh {}
		Intern,LiveUpdateCmd {}
	}
	# delete comments from array
	array unset config {#}

	proc configure {opt val} {
		variable config
		if {[info exists config($opt)]} {
			set config($opt) $val
			return 0
		} else {
			return -code error "Configuration option $opt does not exist"
		}
	}

	# create a new configuration option
	proc create {opt val} {
		variable config
		set config($opt) $val
	}

	proc delete {opt} {
		catch {array unset config $opt}
	}

	proc cget {opt} {
		variable config
		if {[info exists config($opt)]} {
			return $config($opt)
		} else {
			return {}
		}
	}

	# same as configure, but append $val to the list $opt
	proc append {opt val} {
		variable config
		if {[info exists config($opt)]} {
			lappend config($opt) $val
			return 0
		} else {
			return -code error "Configuration option $opt does not exist"
		}
	}

	# load configuration from file
	proc load {} {
		variable config
		# check in the following locations: 
		# ~/.esweep/impulse.conf
		# [pwd]/impulse.conf
		set filename [glob ~]/.esweep/impulse.conf
		if {[catch {set fp [open $filename r]}]} {
			set filename [pwd]/esweep/impulse.conf
			if {[catch {set fp [open $filename r]}]} {
				# there's no config file, stick with defaults
				return 1
			}
		}
		# keep this filename
		set config(Filename) $filename
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

	proc save {} {
		variable config
		# try to save the configuration in the file $config(Filename)
		# If $config(Filename) is empty, create the configuration
		# in one of these locations (first one wins): 
		# ~/.esweep/impulse.conf
		# [pwd]/impulse.conf
		if {$config(Filename) eq {}} {
			set config(Filename) [glob ~]/.esweep/impulse.conf
			if {[catch {set fp [open $config(Filename) w]}]} {
				set config(Filename) [pwd]/esweep/impulse.conf
				if {[catch {set fp [open $config(Filename) w]}]} {
					# unable to open one of the standard locations
					return -code error {Could not store configuration!}
				}
			}
		} else {
			if {[catch {set fp [open $config(Filename) w]}]} {
				# unable to open one of the standard locations
				return -code error {Could not store configuration!}
			}
		}
		puts $fp [array get config]
		close $fp
	}

	proc trace {op cmd args} {
		foreach var $args {
			::trace add variable ::impulse::config::config($var) $op $cmd
		}	
	}

	namespace ensemble create -subcommands [list create configure append cget delete load save trace]
}

