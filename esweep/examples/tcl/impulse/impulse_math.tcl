# This module provides all the math necessary to calculate the impulse reponse from measured
# signals, calculates the frequency response and, if the IR is measured with a logarithmic sweep, 
# it also calculates the harmonic distortions with the Farina method. 

namespace eval ::impulse::math {
	proc scale {sig meas_mode scale_mode reference out_sense in_sense ref_sense mic_sense} {
		if {$meas_mode eq {Dual}} {
			set sc [expr {$ref_sense*$reference/($in_sense*$mic_sense)}]
		} elseif {$meas_mode eq {Single}} {
			if {$scale_mode eq {Relative}} {
				set sc [expr {$reference/($out_sense*$in_sense*$mic_sense)}]
			} elseif {$scale_mode eq {Absolute}} {
				set sc [expr {$level/($in_sense*$mic_sense)}]
			} else {
				return -code error "Unknown scaling mode $scale_mode"
			}
		} else {
				return -code error "Unknown measurement mode $meas_mode"
		}
		esweep::expr sig * $sc
		return $sc
	}

	proc calcIR {meas_mode sig sys ref} {
		set ir [esweep::clone -src $sys]
		if {$meas_mode eq {Dual}} {
			esweep::deconvolve -signal ir  -filter $ref
		} elseif {$meas_mode eq {Single}} {
			esweep::deconvolve -signal ir  -filter $sig
		} else {
				return -code error "Unknown measurement mode $meas_mode"
		}
		esweep::toWave -obj ir
		return $ir
	}

	proc msToSamples {val sr} {
		return [expr {int(0.5+$val*$sr/1000.0)}]
	}

	proc samplesToMs {val sr} {
		return [expr {1000.0*$val/$sr}]
	}

	proc calcFR {ir gate_start gate_stop fft_length smoothing} {
		set samplerate [esweep::samplerate -obj $ir]

		# convert $gate_start to samples
		set gate_start [msToSamples $gate_start $samplerate]
		set gate_stop [msToSamples $gate_stop $samplerate]
		set gate_length [expr {$gate_stop - $gate_start}]
		if {$fft_length == 0} {
			set fft_length [expr {int(0.5+pow(2, int(0.5+ceil(log10($gate_length)/log10(2)))))}]
		} else {
			if {$fft_length < $gate_length} {
				return -code error {Gate length exceeds FFT length}
			}
		}

		set sc [expr {1.0/2e-5}]; # with reference to 20 uPa

		# cut out the IR
		if {$gate_stop >= [esweep::size -obj $ir]} {
			set fr [esweep::create -type [esweep::type -obj $ir] -size $gate_length -samplerate $samplerate]
			set l [esweep::copy -src $ir -dst fr -srcIdx $gate_start]
			esweep::copy -src $ir -dst fr -srcIdx 0 -dstIdx $l
		} else {	
			set fr [esweep::get -obj $ir -from $gate_start -to $gate_stop]
		}
		esweep::toWave -obj fr
		esweep::expr fr * $sc
		esweep::fft -obj fr -inplace
		esweep::toPolar -obj fr
		esweep::lg -obj fr
		esweep::expr fr * 20
		if {$smoothing > 0} {esweep::smooth -obj fr -factor $smoothing}

		return $fr
	}

	proc calcHD {ir sweep_rate distortions gate_start gate_stop fft_length smoothing} {
		set samplerate [esweep::samplerate -obj $ir]
		set result [list]
		set gate_start [msToSamples $gate_start $samplerate]
		set gate_stop [msToSamples $gate_stop $samplerate]
		set gate_length [expr {$gate_stop - $gate_start}]
		foreach k $distortions {
			if {$k < 2} continue
			# find the position of the k'th distortion IR

			set shift [expr {int(0.5+log($k)*$sweep_rate*$samplerate)}]
			set start [expr {$gate_start-$shift}]
			set start [expr {$start < 0 ? [esweep::size -obj $ir] + $start : $start}]

			set max_length [expr {$shift-int(0.5+log($k-1)*$sweep_rate*$samplerate)}]
			set length [expr {$gate_length > $max_length ? $max_length : $gate_length}]

			if {$fft_length == 0} {
			set fft_length [expr {int(0.5+pow(2, int(0.5+ceil(log10($length)/log10(2)))))}]
			} else {
				if {$fft_length < $length} {
					return -code error {Gate length exceeds FFT length}
				}
			}

			set stop [expr {$start+$length}]
			# cut out harmonic impulse response
			# wrap around at end of total IR
			if {$stop >= [esweep::size -obj $ir]} {
				set fr [esweep::create -type [esweep::type -obj $ir] -size $length -samplerate $samplerate]
				set l [esweep::copy -src $ir -dst fr -srcIdx $start]
				esweep::copy -src $ir -dst fr -srcIdx 0 -dstIdx $l
			} else {	
				set fr [esweep::get -obj $ir -from $start -to $stop]
			}
			set sc [expr {1.0/2e-5}]; # with reference to 20 uPa
			# calculate distortion response
			esweep::toWave -obj fr
			esweep::expr fr * $sc
			esweep::fft -obj fr -inplace
			esweep::toPolar -obj fr
			esweep::lg -obj fr
			esweep::expr fr * 20
			if {$smoothing > 0} {esweep::smooth -obj fr -factor $smoothing}

			# Now the trick: to shift the response on the frequency scale, adjust the samplerate
			esweep::setSamplerate -obj fr -samplerate [expr {int(0.5+$samplerate/$k)}]

			lappend result $k [esweep::clone -src $fr]
		}
		return $result
	}

	namespace ensemble create -subcommands [list scale calcIR calcFR calcHD msToSamples samplesToMs]
}

