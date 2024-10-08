Box2D Benchmark for Android


Trying to build Box2D for Android, I ran into this issue:

https://github.com/erincatto/box2d/issues/813


and I submitted this fix PR:

https://github.com/erincatto/box2d/pull/814


I ran the bench mark in:
```
https://github.com/erincatto/box2d/blob/main/benchmark/main.c
```

on a Samsung Galaxy A13. This is a 32-bit ARM7 device.


I adapted [Raymob](https://github.com/Bigfoot71/raymob) and also made some modifications to `main.c` to allow running on Android.



I did 2 runs:
* with BOX2D_ENABLE_SIMD=ON
* with BOX2D_ENABLE_SIMD=OFF

and collected the Logcat transcript and .csv files in the `output` folder.



