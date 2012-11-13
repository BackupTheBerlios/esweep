load ../../../libesweeptcl.so esweep

array set config {
	Audio,Device 		audio:/dev/audio
	Audio,Output,Channel 	1
	Audio,Input,RefChannel 	2
	Audio,Input,DUTChannel 	1
	Audio,Samplerate 	48000

	Measurement,F1		20
	Measurement,F2		20e3
	Measurement,Step	12
	Measurement,Reference	7.2
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
       	# let the audio subsystem settle itself
	# on usual platforms 10 frames are enough
	set frames 10
	set framesize [esweep::audioQuery -handle $au -param framesize]
	set samplerate [esweep::audioQuery -handle $au -param samplerate]
	set delay [expr {int(0.5+1000*$framesize/$samplerate)/2}]

	set offset 0
	set old_offset 0
	while {$frames > 0} {
		esweep::audioIn -handle $au -signals $sig_in -offset $old_offset
		set offset [esweep::audioOut -handle $au -signals $sig_out -offset $old_offset]
		if {$offset > $old_offset} {incr frames -1}
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
		if {$offset > $old_offset} {incr frames -1}
		# allow wrap around
		if {$offset >= $sigsize} {set offset 0}
		set old_offset $offset
		after $delay
	}
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

proc measure_quick {} {
	# create the multitone signal
	set out [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
	set sine [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
	expr {srand(1)}
	for {set i 0} {$i < $n} {incr i} {
		set phase [expr {rand()*acos(-1)}]
		lappend bins [esweep::generate sine -obj sine -frequency $f -phase $phase]
		esweep::expr out + $sine
		set f [expr $f $step]
	}
	# FFT of DUT signal
	esweep::fft -inplace -obj dut
	# FFT of Ref signal
	esweep::fft -inplace -obj ref

	# get impedance, phase, real and imaginary part for each frequency bin
	foreach bin $bins {
		lassign [esweep::index -obj $dut -index $bin] df dr di
		lassign [esweep::index -obj $ref -index $bin] rf rr ri
		# difference voltage
		set udr [expr {$rr-$dr}]
		set udi [expr {$ri-$di}]
		# complex division
		set re [expr {($dr*$udr+$di*$udi)/($udr**2+$udi**2)}]
		set im [expr {($di*$udr-$dr*$udi)/($udr**2+$udi**2)}]
		# scaling by reference resistor
		set re [expr {$config(Measurement,Reference)*$re}]
		set im [expr {$config(Measurement,Reference)*$im}]

		set f [expr {1.0*$bin*$samplerate/$fftsize}]
		lappend result $f [expr {sqrt($re**2+$im**2)}] [expr {atan2($im, $re)}] $re $im
	}
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
		# calculate the FFT size that the deviation from the desired frequency is below 1%
		set dev [expr {0.01*$f}]
		set fftsize [expr {int(0.5+pow(2, ceil(log(1.0*$samplerate/$dev)/log(2))))}]
		set fftsize [expr {$fftsize < $framesize ? $framesize : $fftsize}]

		set out [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
		esweep::generate sine -obj out -frequency $f

		# generate sine and cosine for DFT
		set cos [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
		esweep::generate sine -obj cos -frequency $f -phase [expr {acos(0)}]
		set sin [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
		esweep::generate sine -obj sin -frequency $f
		
		# reduce output level to -3 dB peak
		esweep::expr out / [expr {[esweep::max -obj $out]*sqrt(2)}]

		# generate input signals
		set in1 [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]
		set in2 [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $fftsize]

		# record signal
		esweep::audioSync -handle $au
		record $au [list $out $out] [list $in1 $in2] $fftsize

		if {$config(Audio,Input,DUTChannel) == 1} {
			set dut $in1
			set ref $in2
		} else {
			set dut $in2
			set ref $in1
		}

		# do DFT
		set dr [esweep::clone -src $dut]
		set di [esweep::clone -src $dut]
		set rr [esweep::clone -src $ref]
		set ri [esweep::clone -src $ref]

		esweep::expr dr * $cos
		esweep::expr rr * $cos
		esweep::expr di * $sin
		esweep::expr ri * $sin

		set dr [esweep::sum -obj $dr]
		set di [esweep::sum -obj $di]
		set rr [esweep::sum -obj $rr]
		set ri [esweep::sum -obj $ri]

		# calculate complex impedance
		# difference voltage
		set udr [expr {$rr-$dr}]
		set udi [expr {$ri-$di}]
		# complex division
		set re [expr {($dr*$udr+$di*$udi)/($udr**2+$udi**2)}]
		set im [expr {($di*$udr-$dr*$udi)/($udr**2+$udi**2)}]
		# scaling by reference resistor
		set re [expr {$config(Measurement,Reference)*$re}]
		set im [expr {$config(Measurement,Reference)*$im}]
		lappend result $f [expr {sqrt($re**2+$im**2)}] [expr {atan2($im, $re)}] $re $im

		set f [expr $f $step]
	}
	# close audio device
	esweep::audioSync -handle $au
	esweep::audioClose -handle $au

	return $result
}

foreach {f abs arg re im} [measure] {
	set f [format "%8.3f" $f]
	set abs [format "%8.3f" $abs]
	set arg [format "%8.3f" [expr {180*$arg/acos(-1)}]]
	set re [format "%8.3f" $re]
	set im [format "%8.3f" $im]
	puts stdout "$f $abs $arg $re $im"
}
