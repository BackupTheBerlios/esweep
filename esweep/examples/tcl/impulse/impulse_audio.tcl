# The audio part of the impulse applications
# open audio device, measure the impulse response, play a test tone

namespace eval ::impulse::audio {
	set play 0
	set au_hdl {}

	proc open {device {samplerate 48000} {bitdepth 16} {out_channel {2}} {in_channel {2}}} {
		variable au_hdl
		if {[catch {set hdl [esweep::audioOpen -device $device]}]} {
				return -code error {Error opening audio device. Check configuration.}
		}
		if {[esweep::audioConfigure -handle $hdl -param {precision} -value $bitdepth] < 0} {
			esweep::audioClose -handle $hdl
			return -code error {Error configuring bitdepth on audio device. Check configuration.} 
		}
		if {[esweep::audioConfigure -handle $hdl -param {samplerate} -value $samplerate] < 0} {
			esweep::audioClose -handle $hdl
			return -code error {Error configuring samplerate on audio device. Check configuration.} 
		}
		# Usually, we open the device with 2 channels. If one of the input or output channels
		# is greater than 2, try to open more channels
		if {$out_channel > [esweep::audioQuery -handle $hdl -param {play_channels}]} {
			 if {[esweep::audioConfigure -handle $hdl -param {play_channels} -value $out_channel] < 0} {
				esweep::audioClose -handle $hdl
				return -code error {Error configuring playback channel. Check configuration.} 
			}
		}
		if {$in_channel > [esweep::audioQuery -handle $hdl -param {record_channels}]} {
			 if {[esweep::audioConfigure -handle $hdl -param {record_channels} -value $in_channel] < 0} {
				esweep::audioClose -handle $hdl
				return -code error {Error configuring record channel. Check configuration.} 
			}
		}
			
		return [set au_hdl $hdl]
	}

	proc close {} {
		variable au_hdl
		if {$au_hdl eq {}} {
			return 1
		} else {
			esweep::audioClose -handle $au_hdl
		}
	}

	proc stop {} {
		variable play
		set play 0
		update
	}

	# create a test tone
	proc test {f channel level} {
		variable au_hdl
		variable play
		if {$au_hdl eq {}} {
			return -code error {Audio device not open}
		}
		set samplerate [esweep::audioQuery -handle $au_hdl -param {samplerate}]
		set framesize [esweep::audioQuery -handle $au_hdl -param framesize]
		# get the frequency
		if {$f eq {}} {
			return -code error {Test tone frequency not defined}
		}

		if {$level < 0.0} { 
			return -code error {Test level < 0}
		}

		# the esweep audio system needs one signal for each channel up to the output channel
		# the unused channels will be zero
		set out [list]
		for {set i 1} {$i <= $channel} {incr i} {
			set signal [esweep::create -type wave -samplerate $samplerate -size $framesize]
			lappend out $signal
		}
		# append the output signal
		set signal [lindex $out end]
		if {[::esweep::generate sine \
			-obj signal \
			-frequency $f] < 0.0} {
				return -code error {Error generating signal. Check configuration.}
		}
		esweep::expr signal * $level

		set delay [expr {int(0.5+250.0*$framesize/$samplerate)}]
		# play
		set play 1
		# synchronize the in/out buffers
		esweep::audioSync -handle $au_hdl
		while {$play} {
			set offset 0
			# create the signal every time new and filter it
			while {$offset < $framesize} {
				set offset [esweep::audioOut -handle $au_hdl -signals $out -offset $offset]
			}
			after $delay
			update
		}
		# close the audio device
		esweep::audioSync -handle $au_hdl
		esweep::audioClose -handle $au_hdl
	}

	proc measure {out_channel in_channel ref_channel level sig_type sig_duration sig_spec sig_locut sig_hicut} {
		variable au_hdl
		variable play

		if {$au_hdl eq {}} {
			return -code error {Audio device not open}
		}
		if {$in_channel == $ref_channel} {
			return -code error {Input and reference channel are identical.}
		}

		if {$level < 0.0} { 
			return -code error {Test level < 0}
		}

		# get the samplerate
		set samplerate [esweep::audioQuery -handle $au_hdl -param samplerate]
		# the desired length of the sweep
		set length [expr {int(0.5+$sig_duration*$samplerate/1000.0)}]
		# the length must be adjusted to the next integer multiple of the framesize
		set framesize [esweep::audioQuery -handle $au_hdl -param framesize]
		set length [expr {int(0.5+ceil(1.0*$length/$framesize)*$framesize)}]

		# the esweep audio system needs one signal for each channel
		# up to the output channel and the input channel, respectively (see below)
		set out [list]
		for {set i 0} {$i < $out_channel} {incr i} {
			lappend out [esweep::create -type wave -samplerate $samplerate -size $length]
		}
	
		set signal [lindex $out end]
		if {[set rate [::esweep::generate \
			$sig_type \
			-obj signal \
			-spectrum $sig_spec \
			-locut $sig_locut \
			-hicut $sig_hicut \
			]] < 0.0} {
				return -code error {Error generating signal.}
		}
		esweep::expr signal * $level

		# The input signal must be long enough to get all of the system response
		# A delay of 200 ms will be enough for any delay
		set totlength [expr {int(0.5+$length+1.0*$samplerate)}]
		set totlength [expr {int(0.5+ceil(1.0*$totlength/$framesize)*$framesize)}]
		# the input signal containers
		set in [list]
		for {set i 0} {$i < [expr {$in_channel > $ref_channel ? $in_channel : $ref_channel}]} {incr i} {
			lappend in [esweep::create -type wave -samplerate $samplerate -size $totlength]
		}
		# set links to the  system and reference response
		set in_sys [lindex $in [expr {$in_channel-1}]]
		set in_ref [lindex $in [expr {$ref_channel-1}]]

		# synchronize the in/out buffers
		esweep::audioSync -handle $au_hdl

		set delay [expr {int(0.5+250.0*$framesize/$samplerate)}]
		set play 1
		# measure
		set offset 0
		# first play and record simultaneously...
		while {$play && $offset < $length} {
			esweep::audioIn -handle $au_hdl -signals $in -offset $offset 
			set offset [esweep::audioOut -handle $au_hdl -signals $out -offset $offset]
			after $delay
			update
		}
		# ...then record the rest
		while {$play && $offset < $totlength} {
			set offset [esweep::audioIn -handle $au_hdl -signals $in -offset $offset] 
			after $delay
			update
		}
		if {!$play} {
			return -code error {Measurement cancelled by user}
		}
		# sync again to clean the audio buffer
		esweep::audioSync -handle $au_hdl

		# close the audio device
		esweep::audioClose -handle $au_hdl

		return [list $signal $in_sys $in_ref $rate]
	}

	namespace ensemble create -subcommands [list open close test measure stop]
}

