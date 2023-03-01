```
$ g++ test1.cpp -o test1 -I..
```

```
$ ./test1 --help
USAGE
    test1 INT1 INT2

OPTIONS
    --help    (default: false)
        Show this help and exit.

    --int-option <integer>   (default: 0)
        An option which takes an integer value.

    --bool-option1    (default: false)
        An option which takes a bool value.

    --[no-]bool-option2    (default: true)
        An option which takes a bool value.

    --[no-]bool-option3    (default: false)
        An option which takes a bool value.

    --[no-]bool-option4    (default: true)
        An option which takes a bool value.

    --size <width>x<height>   (default: 320x240)
        An option which takes a size (WxH).
```

```
$ ./test1
error: the number of arguments is less than expected! (expected at least: 2, given: 0)

USAGE
    test1 INT1 INT2
```

```
$ ./test1 123 456 789
error: the number of arguments is greater than expected! (expected at most: 2, given: 3)

USAGE
    test1 INT1 INT2
```

```
$ ./test1 --int-option
error: the option '--int-option' requires value!
```

```
$ ./test1 --int-option 123 --bool-option1 456 --no-bool-option2 789 --size 99x88
args[0] = 456
args[1] = 789
int_option = 123
bool_option1 = 1
bool_option2 = 0
no_bool_option3 = 1
no_bool_option4 = 0
size.w = 99
size.h = 88
```
