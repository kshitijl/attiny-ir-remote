## Calculating the resistance in series with IR LED

TSAL7400 http://www.mouser.com/catalog/specsheets/tsal7400.pdf
max current: 100mA
forward voltage @ 100mA: 1.35V

During development, I'm using a 5V adapter that I found in my bag of
old junk. I cut off the connector and soldered two jumpers.

V_cc: 5v
So R = (5 - 1.35)/100mA = 36.5ohm

47ohm is the closest value >= 36.5 (to give a safety margin).

For 2 LED's in series,
R = (5 - 2*1.35)/100mA = 23ohm
22 ohm is close enough

With a 3V CR1220,	
R = (3-1.35)/100mA = 16.5ohm
22ohm is the closest I have >= 16.5 ohm

With 2 LED's,
R = (3-2*1.35)/100mA = 3ohm
4.7 ohm is the closest I have.

## Receiving

The lengths of pulses recorded are the same, no matter how much I
change my timescale! I felt trapped in a bizarro fractal universe,
where the remote was broadcasting the same signal at any level of
detail I cared to look at.

More likely, something was wrong with my timer setup code. Eventually
I took a peek at the IR Arduino library's setup code, and noticed that
he set every setup register directly to its value, even `TCNT2`,
whereas I had just assumed that the bootloader (or some other friendly
harness code that runs before mine) would set all registers to 0.

Nope. This isn't Linux any more. Nobody will clean up before you
arrive, and my carelessness was brutally punished with 2 hours of
bizarro fractal debugging. Lesson learned.

## Power struggles

Now that I have an ugly but functional solution I need to think about
how I'm going to power the final version. The receiver sits near the
speakers so it can be plugged into the wall with an AC adapter I
scavenge. That's good because it powers a servo and listens constantly
for incoming signals, both of which cost power.

The remote, on the other hand, obviously needs to carry its own. My
choices are coin battery, AA or AAA batteries. If I really can't get
my shit together then I suppose one of those huge 9v batteries is an
option, but that would be pretty embarrassing.

The `ATTiny85` datasheet has summary data about power consumption:

ATtiny85 
Operating Voltage
– 1.8 - 5.5V for ATtiny25V/45V/85V
– 2.7 - 5.5V for ATtiny25/45/85
• Speed Grade
– ATtiny25V/45V/85V: 0 – 4 MHz @ 1.8 - 5.5V, 0 - 10 MHz @ 2.7 - 5.5V
– ATtiny25/45/85: 0 – 10 MHz @ 2.7 - 5.5V, 0 - 20 MHz @ 4.5 - 5.5V
• Industrial Temperature Range
• Low Power Consumption
– Active Mode:
• 1 MHz, 1.8V: 300 µA
– Power-down Mode:
• 0.1 µA at 1.8V

Crap! Looks like I should have ordered the low-power `ATTiny85V`
variant. Assuming I get one of those, I see two modes, and boy are
they different!

With a CR1220 coin battery running at 3V (measured 3.2V) and a
capacity of 40mAh,
=> 40mAh/100mA = 0.4h
= 1440s of continuous LED discharge (ignoring ATtiny)

When I tried using the coin battery in real life, the range was worse
than what I got using a 5v AC adapter. One obvious reason for this is
that the LED is running with lower current -- the resistor in series
is calibrated to deliver 100mA (less safety margin) at 5v.

But another possible reason for bad range could be that the internal
clock runs slightly off from the measured 8.02MHz at this different
voltage. I will have to measure it and adjust.

AAA battery 1.5V
Typical capacity: 1000mAh

TSOP 1738 pin order: signal ground vcc from front (facing me)


/*
In [1]: 8000000.0/(2*38000)
Out[1]: 105.26315789473684

In [2]: X = 2*39060 * 105

In [3]: X
Out[3]: 8202600

In [4]: 8202600./(2*38000)
Out[4]: 107.92894736842105

Chip was running at 8.2026Mhz, so 108-1 = 107 gave a perfect 38KHz
modulated signal.
 */

On power down mode at 5V, I measured a current of 0.5uA (for the whole
circuit). That gives me 80,000 hours with the tiny CR220 battery,
assuming current consumption is the same at a lower voltage
(unlikely).

While a button is pressed, my multimeter reports 15mA. That the value
is lower than 100mA makes sense: just like a 200mA current at a 50%
duty cycle is reported as 100mA by this meter, the same peak current
over the much smaller duty cycle in sending bits (and the timeouts
between bytes) averages out to a lot less. We calculated 1440s of
continuous discharge. Estimating a duty cycle of 15%, we get 9600s of
discharge. Since power consumption in power down mode is a tiny
fraction of this number we can assume that the entire battery will be
consumed due to discharge. Assuming a button is held down for about 3
seconds each time volume is adjusted, we get something like 3200
volume adjustments from this tiny battery.

Is that great or is that terrible? I am such a beginner that I don't
know what is considered awful performance. On a desktop, for example,
doing millions of operations per second (megaflops) of computation
sounds impressive but is actually laughably pitiful. It is several
orders of magnitude away from good. So 3200 button presses sounds
pretty good to me but maybe it's awful.
