# BOMB Lab

## phase 1

It's easy. The answer is a string:
> Border relations with Canada have never been better.

---

## phase 2

In this phase, we should input 6 numbers. Key point:

```asm
mov    -0x4(%rbx),%eax  // now `%eax` is the `Xn`
add    %eax,%eax        // now `%eax` is `Xn * 2`
cmp    %eax,(%rbx)      // `(%rbx)` is the `Xn+1`, means `Xn+1 == Xn * 2`
```

So the answer is:
> 1 2 4 8 16 32

---

## phase 3

In this phase, we should input 2 numbers. The first number is a slector,
and second number is a MAGIC NUMBER:

```
400f71:	mov    0x8(%rsp),%eax       // the first number you input, `slector`
400f75:	jmpq   *0x402470(,%rax,8)   // select the MAGIC NUMBER
400f7c:	mov    $0xcf,%eax              // slector == 0, MAGIC == 0xcf == 207
400f81:	jmp    400fbe <phase_3+0x7b>
400f83:	mov    $0x2c3,%eax             // slector == 2, MAGIC == 0x2c3 == 707
400f88:	jmp    400fbe <phase_3+0x7b>
400f8a:	mov    $0x100,%eax             // slector == 3, MAGIC == 0x100 == 256
400f8f:	jmp    400fbe <phase_3+0x7b>
400f91:	mov    $0x185,%eax             // slector == 4, MAGIC == 0x185 == 389
400f96:	jmp    400fbe <phase_3+0x7b>
400f98:	mov    $0xce,%eax              // slector == 5, MAGIC == 0xce == 206
400f9d:	jmp    400fbe <phase_3+0x7b>
400f9f:	mov    $0x2aa,%eax             // slector == 6, MAGIC == 0x2aa == 682
400fa4:	jmp    400fbe <phase_3+0x7b>
400fa6:	mov    $0x147,%eax             // slector == 7, MAGIC == 0x147 == 327
400fab:	jmp    400fbe <phase_3+0x7b>
400fad:	callq  40143a <explode_bomb>
400fb2:	mov    $0x0,%eax
400fb7:	jmp    400fbe <phase_3+0x7b>
400fb9:	mov    $0x137,%eax             // slector == 1, MAGIC == 0x137 == 311
400fbe:	cmp    0xc(%rsp),%eax        // the second number you input, `MAGIC`
400fc2:	je     400fc9 <phase_3+0x86>
400fc4:	callq  40143a <explode_bomb>
400fc9:	add    $0x18,%rsp
400fcd:	retq
```

So the answer is one of fllow:

> 0 207
> 1 311
> 2 707
> 3 256
> 4 389
> 5 206
> 6 682
> 7 327

---

## phase 4

In this phase, we should input 2 numbers. The second number must be `0`:
`cmpl   $0x0,0xc(%rsp)`

It'll call `func4(x, 0, 0xe)`, where `x` is the first number you input,
and it should less than or equal to  `0xe`:

```asm
cmpl   $0xe,0x8(%rsp)
jbe    40103a <phase_4+0x2e>
callq  40143a <explode_bomb>
```

And the return value of `func4`should be `0`:

```asm
callq  400fce <func4>
test   %eax,%eax
jne    401058 <phase_4+0x4c>
```

We can rewrite `func4` with ***C***, and test which `x (0≤x≤14)` will make `func4` return `0`:

```c
int func(int di, int si, int dx)
{
    int ax = dx -si;
    int cx = ax;

    cx = (unsigned int)cx >> 0x1f;

    ax = ax + cx;
    ax = ax >> 1;
    cx = si + ax;

    if (cx > di) {
        dx = cx - 1;
        ax = func(di, si, dx);
        ax = ax + ax;
        return ax;
    }
    
    ax = 0;

    if (cx >= di) return ax;

    si = cx + 1;
    ax = func(di, si, dx);
    ax = ax + ax + 1;

    return ax;
}

int main()
{
    for (int i = 0; i < 0xf; i++) {
        int ret = func(i, 0, 0xe);
        printf("%2d -> %d", i, ret);
        
        if (ret == 0) {
            printf("    [y]");
        }
        printf("\n");
    }

    return 0;
}
```

We will get this:

>  0 -> 0    [y]
>  1 -> 0    [y]
>  2 -> 4
>  3 -> 0    [y]
>  4 -> 2
>  5 -> 2
>  6 -> 6
>  7 -> 0    [y]
>  8 -> 1
>  9 -> 1
> 10 -> 5
> 11 -> 1
> 12 -> 3
> 13 -> 3
> 14 -> 7

So the first number can be one of `0 1 3 7`. The answer is:

> [0 1 3 7] 0

---

## phase 5

In this phase, we should input a `string` of length `6`:

```asm
callq  40131b <string_length>
cmp    $0x6,%eax
je     4010d2 <phase_5+0x70>
```

在对输入的字符串`进行处理`之后，和一个`给定的字符串`进行比较，如果不一致，则`引爆炸弹`:

```asm
mov    $0x40245e,%esi    // the given string
lea    0x10(%rsp),%rdi   // your string
callq  401338 <strings_not_equal>
```

给定的字符串在地址`0x40245e`处，可以在`gdb`中打印出来:

> (gdb) x /8xb 0x40245e
> 0x40245e:       0x66    0x6c    0x79    0x65    0x72    0x73    0x00    0x00

这个给定的字符串就是`flyers`，但是答案并不是`flyers`。我们输入的字符串经过处理之后应该变成`flyers`。

处理流程如下:
对于输入的每个字符`x`，取其最低4位作为`index`，即`index = x & 0x0f`。取出地址`0x4024b0 + index`处的
一个字节，赋给`x`，即`x = *((char *)(0x4024b0 + index))`。

可以打印出地址`0x4024bo`开始的内容:

> (gdb) x /16xb 0x4024b0
> 0x4024b0:  0x6d    0x61    0x64    0x75    0x69    0x65    0x72    0x73
> 0x4024b8:  0x6e    0x66    0x6f    0x74    0x76    0x62    0x79    0x6c

也就是说，我们要从上面16个字节中挑出`flyers`这6个字符。所以，`index`分别为: `0x9 0xf 0xe 0x5 0x6 0x7`。
因此，我们要输入的6个字符中，第1个字符的`ASCII`码的低4位为`0x9`, 第2个字符的`ASCII`码的低4位为`0xf`,
以此类推。

So the answer is:

> the 1st ASCII character can be one of: `) 9 I Y i y`
> the 2nd ASCII character can be one of: `/ ? O _ o`
> the 3rd ASCII character can be one of: `. > N ^ n ~`
> the 4th ASCII character can be one of: `% 5 E U e u`
> the 5th ASCII character can be one of: `& 6 F V f v`
> the 6th ASCII character can be one of: `' 7 G W g w`
>
> e.g. the input string can be: `yonufg`, `)/.%&'`, or `9?^567`

---

## phase 6

在这个阶段需要输入`6`个数字，这6个数字小于等于*6*且互不相等:

```asm
mov    0x0(%r13),%eax
sub    $0x1,%eax
cmp    $0x5,%eax
jbe    401128 <phase_6+0x34>
```

在做完上述判断之后，会对读取到的6个数字做一个处理，即对每个输入的数字`n`，
令`n = 7 - n`。这算是一个伏笔，之后会再提到:

```asm
mov    $0x7,%ecx
mov    %ecx,%edx
sub    (%rax),%edx
mov    %edx,(%rax)
```

接下来是一段非常诡异的代码:
```
40116f:	mov    $0x0,%esi
401174:	jmp    401197 <phase_6+0xa3>

401176:	mov    0x8(%rdx),%rdx
40117a:	add    $0x1,%eax
40117d:	cmp    %ecx,%eax
40117f:	jne    401176 <phase_6+0x82>
401181:	jmp    401188 <phase_6+0x94>
401183:	mov    $0x6032d0,%edx           // `0x6032d0` is the key point!
401188:	mov    %rdx,0x20(%rsp,%rsi,2)
40118d:	add    $0x4,%rsi
401191:	cmp    $0x18,%rsi
401195:	je     4011ab <phase_6+0xb7>

401197:	mov    (%rsp,%rsi,1),%ecx     // load the nember you input one by one
40119a:	cmp    $0x1,%ecx
40119d:	jle    401183 <phase_6+0x8f>
40119f:	mov    $0x1,%eax
4011a4:	mov    $0x6032d0,%edx
4011a9:	jmp    401176 <phase_6+0x82>

4011ab:	mov    0x20(%rsp),%rbx
4011b0:	lea    0x28(%rsp),%rax
4011b5:	lea    0x50(%rsp),%rsi
4011ba:	mov    %rbx,%rcx
4011bd:	mov    (%rax),%rdx
4011c0:	mov    %rdx,0x8(%rcx)
4011c4:	add    $0x8,%rax
4011c8:	cmp    %rsi,%rax
4011cb:	je     4011d2 <phase_6+0xde>
4011cd:	mov    %rdx,%rcx
4011d0:	jmp    4011bd <phase_6+0xc9>
```

上述的代码片段中有一个地址`0x6032d0`，我们可以在`gdb`里把这个地址开始的内容打印出来:

> (gdb) x /16xg 0x6032d0
> 0x6032d0 \<node1>:       0x000000010000014c      0x00000000006032e0
> 0x6032e0 \<node2>:       0x00000002000000a8      0x00000000006032f0
> 0x6032f0 \<node3>:       0x000000030000039c      0x0000000000603300
> 0x603300 \<node4>:       0x00000004000002b3      0x0000000000603310
> 0x603310 \<node5>:       0x00000005000001dd      0x0000000000603320
> 0x603320 \<node6>:       0x00000006000001bb      0x0000000000000000

从地址`0x6032d0`开始的内容，前4个字节是序列号(1~6)，接下来4个字节是随机数，然后跟着8个字节表示
一个地址，并且该地址刚好是接下来的下一个`node`的地址。

到这已经非常清晰了，上面的数据其实表征了一个非常常用的数据结构:`链表`，形式如下:

```c
struct node {
  int value;          // the node's value
  int n;              // the node's serial number
  struct node *next;  // point to next node
};
```

上述数据的组织形式如下:

> +---+-------+--------+   +---+-------+--------+   +---+-------+--------+
> | 1 | 0x14c | 0x32e0 |-->| 2 |  0xa8 | 0x32f0 |-->| 3 | 0x39c | 0x3300 |
> +---+-------+--------+   +---+-------+--------+   +---+-------+--------+
>                                                                      | 
>                                                                      V
> +--------+-------+---+   +--------+-------+---+   +--------+-------+---+
> | 0x0000 | 0x1bb | 6 |<--| 0x3320 | 0x1dd | 5 |<--| 0x3310 | 0x2b3 | 4 |
> +--------+-------+---+   +--------+-------+---+   +--------+-------+---+

因此，上述那段`诡异的代码`其实就是在对链表排序。每个`node`的`n`和`value`不发生改变，
只是改变`next`指针的值，达到`从大到小`的排序效果。

如果按照从大到小的顺序(比较每个`node`的`value`)，则`node`序列号应该为`3 4 5 6 1 2`。
但是，如果输入以上数字序列，还是会`引爆炸弹`。

还记得前面提到的`伏笔`吗？在读取输入的数字后，会令`n = 7 - n`，所以答案应该是:

> 4 3 2 1 6 5

