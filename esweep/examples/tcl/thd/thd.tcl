load ../../../libesweeptcl.so esweep

array set config {
	Audio,Device 		audio:/dev/audio1
	Audio,Samplerate 	44100
	Audio,Output,Channel 	1
	Audio,Output,Max	0.9
	Audio,Input,Channel 	1
	Audio,Input,Cal		0.29
	Audio,Input,Cal,Base	2e-5

	Measurement,F1		700
	Measurement,F2		3000
	Measurement,Step	3
	Measurement,Harmonics	{2 3 5}
}

# get options

proc openAudioDevice {device samplerate framesize} {
	if {[catch {set au [esweep::audioOpen -device $device]}]} {
		puts stderr {Unable to open audio device. Exiting.}
		exit
	}
	if {[catch {esweep::audioConfigure -handle $au -param samplerate -value $samplerate}]} {
		puts stderr {Unsupported samplerate.}
		exit
	}
	if {[catch {esweep::audioConfigure -handle $au -param framesize -value $framesize}]} {
		return -code error
	}

	return $au
}

proc record {au sig_out sig_in sigsize} {
	set maxFrames 20
	set framesize [esweep::audioQuery -handle $au -param framesize]
	set samplerate [esweep::audioQuery -handle $au -param samplerate]
	set delay [expr {int(0.5+1000*$framesize/$samplerate)/4}]
	set offset 0
	set old_offset 0

	# fill input and output buffers
	set frames 0
	esweep::audioSync -handle $au
	while {$frames < $maxFrames} {
		esweep::audioIn -handle $au -signals $sig_in -offset $old_offset
		set offset [esweep::audioOut -handle $au -signals $sig_out -offset $old_offset]
		if {$offset > $old_offset} {
			incr frames
		}
		# allow wrap around
		if {$offset >= $sigsize} {set offset 0}
		set old_offset $offset
		after $delay
	}

	# now do the actual recording
	set frames [expr {$sigsize/$framesize}]
	while {$frames > 0} {
		esweep::audioIn -handle $au -signals $sig_in -offset $old_offset
		set offset [esweep::audioOut -handle $au -signals $sig_out -offset $old_offset]
		if {$offset > $old_offset} {
			incr frames -1
		}
		# allow wrap around
		if {$offset >= $sigsize} {set offset 0}
		set old_offset $offset
		after $delay
	}
	# make sure that we stop playback at the end of the output signal
	if {$offset != 0} {
		while {$offset < $sigsize} {
			set offset [esweep::audioOut -handle $au -signals $sig_out -offset $offset]
		}
	}
	esweep::audioSync -handle $au
	return
}

# calculate the step variable for linear or logarithmic spacing
proc getStep {from to n spacing} {
	if {$spacing eq {lin}} {
		set step "+ [expr {double($to - $from)/($n-1)}]"
	} elseif {$spacing eq {log}} {
		set step "* [expr {pow(10, log10(double($to)/$from)/($n-1))}]"
	} else {
		puts stderr "Error: unknown spacing \"$spacing\""
		exit
	}	
	return $step
}

proc measure {} {
	global config
	# open audio device with fixed framesize 2048
	# this power-of-2 is necessary for later FFT
	set au [openAudioDevice $config(Audio,Device) $config(Audio,Samplerate) 2048]
	set framesize [esweep::audioQuery -handle $au -param framesize]
	set samplerate $config(Audio,Samplerate)

	set n [expr {int(0.5+$config(Measurement,Step)*log($config(Measurement,F2)/$config(Measurement,F1))/log(2))}]
	set step [getStep $config(Measurement,F1) $config(Measurement,F2) $n log]

	set f $config(Measurement,F1)
	for {set i 0} {$i < $n} {incr i} {
		esweep::audioSync -handle $au
		# calculate the FFT size that the deviation from the desired frequency is below 1%
		set dev [expr {0.01*$f}]
		set fftsize [expr {int(0.5+pow(2, ceil(log(1.0*$samplerate/$dev)/log(2))))}]
		set fftsize [expr {$fftsize < $framesize ? $framesize : $fftsize}]

		set out [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
		esweep::generate sine -obj out -frequency $f

		foreach k [concat 1 $config(Measurement,Harmonics)] {
			# generate sine and cosine for DFT
			if {$k*$f >= $config(Audio,Samplerate)/2} continue
			set cos($k) [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
			esweep::generate sine -obj cos($k) -frequency [expr {$k*$f}] -phase [expr {acos(0)}]
			set sin($k) [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
			esweep::generate sine -obj sin($k) -frequency [expr {$k*$f}] 
		}
		
		# reduce output level to -3 dB peak
		esweep::expr out / [expr {[esweep::max -obj $out]*sqrt(2)}]

		# generate input signals
		set in1 [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
		set in2 [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]

		# record signal
		record $au [list $out $out] [list $in1 $in2] $fftsize

		if {$config(Audio,Input,Channel) == 1} {
			set dut $in1
		} else {
			set dut $in2
		}

		# do DFT for each harmonic
		lappend result $f
		set HD [list]
		foreach k [concat 1 $config(Measurement,Harmonics)] {
			if {$k*$f >= $config(Audio,Samplerate)/2} continue
			set re [esweep::clone -src $dut]
			set im [esweep::clone -src $dut]

			esweep::expr re * $cos($k)
			esweep::expr im * $sin($k)

			set re [esweep::sum -obj $re]
			set im [esweep::sum -obj $im]

			lappend HD [expr {20*log10(2*sqrt($re**2+$im**2)/($config(Audio,Input,Cal)*$config(Audio,Input,Cal,Base)*$fftsize))}]
		}
		lappend result $HD

		set f [expr $f $step]
	}
	# close audio device
	esweep::audioClose -handle $au

	return $result
}

puts -nonewline stdout {# f/Hz}
foreach k [concat 1 $config(Measurement,Harmonics)] {
	puts -nonewline stdout "\tK$k"
	lappend HD $k
}
# add linefeed
puts stdout {}
foreach {f HD} [measure] {
	set f [format "%8.3f" $f]
	puts -nonewline stdout $f
	foreach k $HD {
		set k [format "%8.3f" $k]
		puts -nonewline stdout "\t$k"
	}
	puts stdout {}
}

