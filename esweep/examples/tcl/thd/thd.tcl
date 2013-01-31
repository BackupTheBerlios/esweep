load ../../../libesweeptcl.so esweep

array set config {
	Audio,Device 		audio:/dev/audio1
	Audio,Samplerate 	44100
	Audio,Output,Channel 	1
	Audio,Output,Max	0.9
	Audio,Input,Channel 	1
	Audio,Input,Cal		0.087
	Audio,Input,Cal,Base	2e-5

	Measurement,F1		50
	Measurement,F2		10000
	Measurement,Step	6
	Measurement,Harmonics	{2 3 5}
	Measurement,IntraBurstDelay 200
	Measurement,MaxWait	2000
	Measurement,LvlAcc	0.1
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

proc record {au sig_out sig_in sigsize useChan maxWait waitThres} {
	set framesize [esweep::audioQuery -handle $au -param framesize]
	set samplerate [esweep::audioQuery -handle $au -param samplerate]
	set delay [expr {int(0.5+1000*$framesize/$samplerate)/4}]
	set offset 0
	set old_offset 0
	set avgFrames 3

	set in1 [esweep::create -type wave -size $framesize -samplerate $samplerate]
	set in2 [esweep::create -type wave -size $framesize -samplerate $samplerate]
	if {$useChan == 1} {
		set useChan $in1
	} else {
		set useChan $in2
	}
	set in [list $in1 $in2]
	esweep::audioSync -handle $au
	# fill input and output buffers, this usually takes 6 frames
	# increase this when necessary, e. g. with WinMM output and its high delay
	# 10 frames seem necessary
	set frames 0
	while {$frames < 6} {
		esweep::audioIn -handle $au -signals $in
		set offset [esweep::audioOut -handle $au -signals $sig_out -offset $old_offset]
		# only count full frames
		if {$offset - $old_offset >= $framesize} {
			incr frames
		}
		# allow wrap around
		if {$offset >= $sigsize} {set offset 0}
		set old_offset $offset
		# this short delay reduces CPU load
		after $delay
	}

	# wait for signal to settle
	# This is done by a calculating a running average over the last $avgFrames frames
	# and comparing the current frame with it; if the level difference is below the threshold
	# start recording.
	
	# Step 1: get $avgFrames frames
	set frames 0
	set lvlList [list]
	set avg 0
	while {$frames < $avgFrames} {
		esweep::audioIn -handle $au -signals $in
		set offset [esweep::audioOut -handle $au -signals $sig_out -offset $old_offset]
		if {$offset - $old_offset >= $framesize} {
			set lvl [esweep::sqsum -obj $useChan]
			set avg [expr {$avg+$lvl}]
			lappend lvlList $lvl
			incr frames
		}
		if {$offset >= $sigsize} {set offset 0}
		set old_offset $offset
		after $delay
	}

	# Step 2: wait for signal to settle, but only $maxWait milliseconds (converted to frames)
	set maxWait [expr {int(0.5+$maxWait*$samplerate/(1000.0*$framesize))}]
	if {$maxWait < 2} {set maxWait 2}
	# square the threshold because we are working with the square sum here
	set waitThres [expr {$waitThres**2}]
	set frames 0
	while {$frames < $maxWait} {
		esweep::audioIn -handle $au -signals $in
		set offset [esweep::audioOut -handle $au -signals $sig_out -offset $old_offset]
		if {$offset - $old_offset >= $framesize} {
			set lvl [esweep::sqsum -obj $useChan]
			set idx [expr {$frames % $avgFrames}]
			set avg [expr {$avg-[lindex $lvlList $idx]+$lvl}]
			lset lvlList $idx $lvl
			incr frames
		}
		if {$offset >= $sigsize} {set offset 0}
		set old_offset $offset
		# check for threshold
		if {abs((3*$lvl - $avg)/$avg) < $waitThres} {
			break
		}
		after $delay
	}

	# now do the actual recording
	set frames [expr {$sigsize/$framesize}]
	while {$frames > 0} {
		esweep::audioIn -handle $au -signals $sig_in -offset $old_offset
		set offset [esweep::audioOut -handle $au -signals $sig_out -offset $old_offset]
		if {$offset - $old_offset >= $framesize} {
			incr frames -1
		}
		# allow wrap around
		if {$offset >= $sigsize} {set offset 0}
		set old_offset $offset
		after $delay
	}
	# make sure that we stop playback at the end of the output signal
	# this reduces popping noises
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
		set bin [esweep::generate sine -obj out -frequency $f]

		foreach k [concat 1 $config(Measurement,Harmonics)] {
			# generate sine and cosine for DFT
			if {$k*$f >= $config(Audio,Samplerate)/2} continue
			# esweep generates a sine that it fits with a integer number of periods
			# in the supplied object. This may give not the desired harmonic here. By fiddling
			# with the frequency parameter we need to make sure that it is always the correct 
			# frequecy bin. 
			set fd [expr {1.0*$k*$bin*$samplerate/$fftsize}]
			set cos($k) [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
			esweep::generate sine -obj cos($k) -frequency $fd -phase [expr {acos(0)}]
			set sin($k) [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
			esweep::generate sine -obj sin($k) -frequency $fd
		}
		
		# reduce output level to -3 dB peak
		esweep::expr out / [expr {[esweep::max -obj $out]*sqrt(2)}]

		# generate input signals
		set in1 [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
		set in2 [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]

		# record signal
		record $au [list $out $out] [list $in1 $in2] $fftsize $config(Audio,Input,Channel) \
				$config(Measurement,MaxWait) $config(Measurement,LvlAcc)

		if {$config(Audio,Input,Channel) == 1} {
			set dut $in1
		} else {
			set dut $in2
		}
		set fft [esweep::clone -src $dut]

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
		# intra burst delay
		after $config(Measurement,IntraBurstDelay)
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

