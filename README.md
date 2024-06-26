Serial Reaction Time Task with a probabilistic sequence, similar to [Kaufman et al.](https://www.sciencedirect.com/science/article/abs/pii/S001002771000123X)'s (2010) task.

![](assets/demo.gif)

Default keys: `c`, `v`, `b`, `n`.

```console
$ gcc -o srtt srtt.c -lraylib
```

* reaction time measured in microseconds
* sequences' probability can be changed (`.85`/`.15` by default)
* sequences can be automatically generated (1st or 2nd-order conditional)
* number of possible locations can be changed [but don't expect too much for now]
* automatic results saving

```console
$ cat results_RANDOMID_date_time.txt
block;item;reaction_time;time_from_start;answer;reg_item;irreg_item;shown_item;seq_shown;030231321012;013102123032
0;0;847851;847919;3;3;1;3,0
0;1;261877;1360004;1;1;2;1,0
0;2;213786;1823961;0;0;2;0,0
0;3;237840;2312008;2;2;3;2,0
0;4;293771;2855967;1;1;3;1,0
0;5;245750;3351906;2;2;3;3,1
...
```

Linux compatible only for now.
