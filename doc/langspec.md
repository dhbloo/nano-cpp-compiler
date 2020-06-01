# Nano-cpp 语言规范

Nano-cpp是C++的一个语言子集，该子集涵盖了C++的部分基本特性，其部分参考了[Embedded C++](https://en.wikipedia.org/wiki/Embedded_C%2B%2B)的语言设计选择。相比C++03标准，特别去除了以下的语言特性，以简化编译器的实现：

+ 异常处理
+ 多重继承
+ 虚基类
+ 命名空间
+ 模版
+ RTTI
+ C++ Style Cast（`static_cast`、`dynamic_cast`、`reinterpret_cast`、`const_cast`、函数型cast/构造如`int(x)`）
+ 联合体（Union）
+ 储存类型修饰符（`auto`、`register`、`static`、`extern`、`mutable`）
+ `using`语句

还去除一些不常用的C/C++特性：

+ CV修饰符中的`volatile`
+ 函数修饰符中的`explicit`
+ `goto`语句
+ 部分运算符重载（`new`、`delete`、`new[]`、`delete[]`），只保留全局`new`与`delete`
+ 位域
+ 可变参数
+ 条件内局部声明

特别的，为了进一步简化实现，仅支持单文件编译（多文件编译作为后续扩展功能实现），因此以下语言特性也一并去除：

+ 头文件引用
+ 预处理宏
+ 链接规范说明（如`extern "C"`）
+ `extern`外部链接修饰符
+ `inline`内联修饰符

总体而言，语言保留的特性主要为C与C++共有常见的部分，加上C++的面向对象语法与一些C++常见的语言特性（如引用）。

以下规范改编自C++03标准（ANSI ISO IEC 14882 2003）。

## 词法

### 标识符

以下定义基于正则表达式：

```
identifier := [_a-zA-Z][_a-zA-Z0-9]*
```

### 字面量

字面量有多种：

```
literal:
	int-literal
	char-literal
	float-literal
	string-literal
	boolean-literal
```

整数字面量只支持10进制与16进制：

```
int-literal:
	decimal-literal
	hexadecimal-literal
```

以下定义基于正则表达式（`<>`为展开其他正则表达式引用）：

```
decimal-literal		:= 0|([1-9][0-9]*)
hexadecimal-literal	:= (0x|0X)[a-fA-F0-9]+
float-literal		:= [-+]?(([0-9]*\.[0-9]+)|([0-9]+\.))(f|F)?
char-literal		:= \'([^\\\'\n]|(\\.))\'
string-literal		:= \"((\\.)|[^"\\\n])*\"
boolean-literal		:= false|true
```

注：浮点型字面量不支持科学计数法。转义字符只支持如下几种：`\' \" \? \\ \a \b \f \n \r \t \v \0`。布尔字面量用到了关键字`false`与`true`。

### 关键字

```
keyword:
bool			break			case			char			class
const			continue		default			delete			do
double			else			enum			false			float
for				friend			if				int				long
new				operator		private			protected		public			
return			short			signed			sizeof			static
struct			switch			this			true			typedef
unsigned		virtual			void			while
```

### 操作符

```
operator-or-punctuator:
{		}		[		]		(		)		;		:		new		delete
?		::		.		.*		+		-		*		/		%		^
&		|		~		!		= 		< 		>		+=		-=		*=
/=		%=		^=		&=		|=		<<		>>		>>=		<<=		==
!=		<=		>=		&&		||		++		--		,		->*		->
```

注：在翻译阶段，每一个操作符被视为一个token。

### 记号

在词法分析中，以上4种类型的符号被视为独立的token。

```
token:
	identifier
	keyword
	literal
	operator-or-punctuator
```

## 语法

语法采用类BNF格式说明：

+ 每一行表示一个选项
+ 方括号表示可选：'[ a ]' 表示a可选，即 "a | ε"

### 1. 上下文相关关键字

新的上下文相关关键字可以被`typedef`、class、enumeration引入：

```
typedef-name:
	identifier
	
class-name:
	identifier
	
enum-name:
	identifier
```

### 2. 基本概念

```
translation-unit:
	[declaration-seq]
```

### 3. 表达式

```
primary-expression:
	literal
	'this'
	'(' expression ')'
	id-expression
	
id-expression:
    unqualified-id
    qualified-id

unqualified-id:
    identifier
    operator-function-id
    conversion-function-id
    '~' class-name

qualified-id:
    ['::'] nested-name-specifier unqualified-id
    '::' identifier
    
nested-name-specifier:
	class-name '::'
	nested-name-specifier class-name '::'

postfix-expression:
    primary-expression
    simple-typename-specifier '{' [expression-list] '}'
    postfix-expression '[' expression ]
    postfix-expression '(' [expression-list] ')'
    postfix-expression '.' id-expression
    postfix-expression '->' id-expression
    postfix-expression '++'
    postfix-expression '--'
    
expression-list:
    assignment-expression
    expression-list ',' assignment-expression

unary-expression:
    postfix-expression
    unary-operator cast-expression
    'sizeof' unary-expression
    'sizeof' '(' type-id ')'
    new-expression
    delete-expression

unary-operator:
    '*' 
    '&' 
    '+' 
    '-' 
    '!' 
    '~'
    '++'
    '--'
    
new-expression:
	'new' [new-placement] new-type-id [new-initializer]
	'new' [new-placement] '(' type-id ')' [new-initializer]

new-placement:
	'(' expression-list ')'

new-type-id:
    type-specifier-seq [new-declarator]

new-declarator:
	ptr-operator-list [direct-new-declarator]
    direct-new-declarator

direct-new-declarator:
    '[' expression ']' 
    direct-new-declarator '[' constant-expression ']'
    
new-initializer:
	'(' [expression-list] ')'

delete-expression:
	'delete' cast-expression
	'delete' '[' ']' cast-expression

cast-expression:
	unary-expression
	'(' type-id ')' cast-expression

pm-expression:
    cast-expression
    pm-expression '.*' cast-expression
    pm-expression '->*' cast-expression

multiplicative-expression:
    pm-expression
    multiplicative-expression '*' pm-expression
    multiplicative-expression '/' pm-expression
    multiplicative-expression '%' pm-expression
    
additive-expression:
    multiplicative-expression
    additive-expression '+' multiplicative-expression
    additive-expression '-' multiplicative-expression

shift-expression:
    additive-expression
    shift-expression '<<' additive-expression
    shift-expression '>>' additive-expression

relational-expression:
    shift-expression
    relational-expression '<' shift-expression
    relational-expression '>' shift-expression
    relational-expression '<=' shift-expression
    relational-expression '>=' shift-expression

equality-expression:
    relational-expression
    equality-expression '==' relational-expression
    equality-expression '!=' relational-expression

and-expression:
    equality-expression
    and-expression '&' equality-expression

exclusive-or-expression:
    and-expression
    exclusive-or-expression '^' and-expression

inclusive-or-expression:
    exclusive-or-expression
    inclusive-or-expression '|' exclusive-or-expression

logical-and-expression:
    inclusive-or-expression
    logical-and-expression '&&' inclusive-or-expression
    
logical-or-expression:
    logical-and-expression
    logical-or-expression '||' logical-and-expression

conditional-expression:
    logical-or-expression
    logical-or-expression '?' expression ':' assignment-expression

assignment-expression:
    conditional-expression
    logical-or-expression assignment-operator assignment-expression

assignment-operator:
	'=' 
	'*=' 
	'/=' 
	'%='
	'+='
    '-=' 
    '>>=' 
    '<<=' 
    '&=' 
    '^=' 
    '|='

expression:
	assignment-expression
	expression ',' assignment-expression

constant-expression:
	conditional-expression
```

注：`assignment-operator`为右结合。在表达式中构造临时对象采用C++11中的`{}`构造表示。

### 4. 语句

```
statement:
    labeled-statement
    expression-statement
    compound-statement
    selection-statement
    iteration-statement
    jump-statement
    declaration-statement

labeled-statement:
    'case' constant-expression ':' statement
    'default' ':' statement

expression-statement:
	[expression] ';'

compound-statement:
	'{' [statement-seq] '}'

statement-seq:
	statement
	statement-seq statement

selection-statement:
    'if' '(' condition ')' statement ['else' statement]
    'switch' '(' condition ')' statement

condition:
    expression
    
iteration-statement:
    'while' '(' condition ')' statement
    'do' statement 'while' '(' expression ')' ';'
    'for' '(' for-init-statement [condition] ';' [expression] ')' statement

for-init-statement:
    expression-statement
    simple-declaration

jump-statement:
    'break' ';'
    'continue' ';'
    'return' [expression] ';'

declaration-statement:
	block-declaration
```

### 5. 声明

```
declaration-seq:
	declaration
	declaration-seq declaration
	
declaration:
	block-declaration
	function-definition
	
block-declaration:
    simple-declaration

simple-declaration:
	decl-specifier-seq [init-declarator-list] ';'
	
decl-specifier:
    type-specifier
    function-specifier
    'friend'
    'typedef'
    
decl-specifier-seq:
	decl-specifier
	decl-specifier-seq decl-specifier
	
function-specifier:
    'virtual'

type-specifier:
    simple-type-specifier
    class-specifier
    enum-specifier
    elaborated-type-specifier
    cv-qualifier
    
simple-type-specifier:
    'char'
    'bool'
    'short'
    'int'
    'long'
    'signed'
    'unsigned'
    'float'
    'double'
    'void'
    
type-name:
    class-name
    enum-name
    typedef-name
    
simple-typename-specifier:
	[nested-name-specifier] type-name
    
elaborated-type-specifier:
	simple-typename-specifier
    class-key [nested-name-specifier] identifier
    'enum' [nested-name-specifier] identifier
    
enum-specifier:
	'enum' [identifier] '{' [enumerator-list] '}'
	
enumerator-list:
	enumerator-definition 
	enumerator-list ',' enumerator-definition
	
enumerator-definition:
	enumerator
	enumerator '=' constant-expression
	
enumerator:
	identifier
```

### 6. 声明符

```
init-declarator-list:
	init-declarator
	init-declarator-list ',' init-declarator

init-declarator:
	declarator [initializer]
	
declarator:
    direct-declarator
    ptr-operator-list direct-declarator

direct-declarator:
    declarator-id
    direct-declarator '(' [parameter-declaration-list] ')' [cv-qualifier]
    direct-declarator '[' [constant-expression] ']'
    '(' declarator ')'

ptr-operator-list:
	ptr-operator
	ptr-operator ptr-operator-list

ptr-operator:
    '*' [cv-qualifier]
    '&'
    nested-name-specifier '*' [cv-qualifier]
    
cv-qualifier:
	'const'

declarator-id:
    id-expression

type-id:
	type-specifier-seq [abstract-declarator]

type-specifier-seq:
	type-specifier [type-specifier-seq]
	
abstract-declarator:
    ptr-operator-list [direct-abstract-declarator]
    direct-abstract-declarator

direct-abstract-declarator:
	[direct-abstract-declarator] '(' [parameter-declaration-list] ')' [cv-qualifier]
	[direct-abstract-declarator] '[' [constant-expression] ']' 
	'(' abstract-declarator ')'

parameter-declaration-list:
    parameter-declaration
    parameter-declaration-list ',' parameter-declaration

parameter-declaration:
    decl-specifier-seq declarator
    decl-specifier-seq declarator '=' assignment-expression
    decl-specifier-seq [abstract-declarator]
    decl-specifier-seq [abstract-declarator] '=' assignment-expression
    
function-definition:
	[decl-specifier-seq] declarator [ctor-initializer] function-body

function-body:
	compound-statement

initializer:
	'=' initializer-clause
	'(' expression-list ')'

initializer-clause:
    assignment-expression
    '{' initializer-list [','] '}'
    '{' '}'

initializer-list:
    initializer-clause
    initializer-list ',' initializer-clause
```

### 7. 类

```
class-specifier:
	class-head '{' [member-specification] '}'

class-head:
    class-key [base-clause]
    class-key [nested-name-specifier] identifier [base-clause]

class-key:
    'class'
    'struct'
    
member-specification:
    member-declaration [member-specification]
    access-specifier ':' [member-specification]

member-declaration:
    decl-specifier-seq [member-declarator-list] ';'
    member-declarator ';'
    function-definition [';']

member-declarator-list:
    member-declarator
    member-declarator-list ',' member-declarator

member-declarator:
    declarator [pure-specifier]
    declarator [constant-initializer]
    
pure-specifier:
	'=' '0'

constant-initializer:
	'=' constant-expression
```

### 8. 派生类

```
base-clause:
	':' base-specifier

base-specifier:
    [nested-name-specifier] class-name
    access-specifier [nested-name-specifier] class-name

access-specifier:
    'private'
    'protected'
    'public'
```

### 9. 特殊成员函数

```
conversion-function-id:
	'operator' conversion-type-id

conversion-type-id:
	type-specifier-seq [conversion-declarator]

conversion-declarator:
	ptr-operator-list

ctor-initializer:
	':' mem-initializer-list

mem-initializer-list:
    mem-initializer
    mem-initializer ',' mem-initializer-list

mem-initializer:
	mem-initializer-id '(' [expression-list] ')'

mem-initializer-id:
	[nested-name-specifier] class-name
	identifier
```

### 10. 运算符重载

```
operator-function-id:
	'operator' operator
	
operator:
    '+' 
    '-' 
    '*' 
    '/' 
    '%' 
    '^' 
    '&' 
    '|' 
    '~'
    '!' 
    '=' 
    '<' 
    '>' 
    '+=' 
    '-=' 
    '*=' 
    '/=' 
    '%='
    '~=' 
    '&=' 
    '|=' 
    '<<' 
    '>>' 
    '>>=' 
    '<<=' 
    '==' 
    '!='
    '<=' 
    '>=' 
    '&&' 
    '||' 
    '++' 
    '--' 
    ',' 
    '->*' 
    '->'
    '(' ')' 
    '[' ']'
```

## 语义规则

注：以下语境中，类泛指`class`或`struct`。本部分的表述并不完全，主要列出一些（可能与原始C++不同的）要点，其他更细致的表述与C++03标准相同，可以参考C++03的标准文档。

### 声明与定义

1. 标识符的声明即是其定义，除了没有函数体的函数声明、`typedef`声明、类名声明、类中的静态成员声明。包含函数体的函数声明为函数定义。对象定义会导致对象的储存空间被保留且对象在合适时候被初始化。
2. 类声明、枚举声明在相同作用域内可以被声明多于一次，但第一次声明之后出现的声明必须与第一次出现的声明相符合。枚举不允许提前声明，即第一次出现的枚举声明应该也是其定义；类可以提前声明。
3. 单一定义原则（One definition rule）：翻译单元中变量、函数、类、枚举不能被定义多于一次。
4. 声明了但没有定义的类、未知长度的数组、不完整类型的数组为*不完整的类型*，不完整的类型与`void`类型均不能用于声明对象。如果存在任何对象定义的类型不完整，则程序是病态的（ill-formed）。不完整的类型如果在之后被定义，则定义之后的类型是完整的。类在其完整定义的结尾`}`之后才被认为是完整类型，在此时前均是不完整类型。（注：不完整对象的引用或指针类型为完整类型，因为其内存大小已知）
6. 当声明修饰符序列（`decl-specifier-seq`）中含有`typedef`时，声明被称作*typedef声明*，此时每个声明符被声明为typedef名字，表示函数声明或对象声明类型的同义词。`typedef`不能与类型修饰符外的修饰符共同使用（包括`static`、`virtual`、`friend`）。`typedef`不能将同一作用域内的类型名再定义为另一种类型。
7. 只有构造函数、析构函数、类型转换函数的`decl-specifier-seq`能被省略。
8. 在`decl-specifier-seq`中，最多只能出现一个类型修饰符（`type-specifier`），除了以下情况：`const`可以与其他类型修饰符使用、`signed`/`unsigned`可以与`char`/`long`/`short`/`int`使用、`short`/`long`可以与`int`使用。以上3种特例中额外的修饰符每例最多只能出现一次。

### 作用域

1. 对象声明的起始作用域在其完整声明后，且在其初始化前（如果有）。对于枚举类型的值标识符，其作用域在枚举值的完整定义之后。
2. 在块内的声明有局部作用域，其作用域在其声明点与块尾之间。局部块内的对象标识符声明可以遮蔽外部的同名对象标识符声明。派生类中定义的对象标识符声明可以遮蔽其继承链上的对象标识符声明。
3. 函数参数的作用域与函数体内的标识符声明的作用域相同。（函数体内不能再声明与函数参数同名的标识符。）
4. 在`if`、`while`、`for`、`switch`条件中声明的标识符作用域与对应的语句体内的标识符声明的作用域相同。
5. 在类中的声明，其作用域不仅包括从定义点到类定义尾，也包括类的所有函数体、函数默认参数、构造函数初始化列表。
6. 类成员名字只能用在类和其派生类作用域内、或其类和派生类对象/对象指针的`.`/`->`运算符后、或其类和派生类名的`::`作用域决议运算符后。
7. 在某一类名、枚举名、`typedef`名有效的作用域中，该类名、枚举名、`typedef`名**不能**被被任何新定义的对象、函数、枚举值标识符遮蔽。

### 名字查找

名字查找将名字的使用与其声明对应，即在多个同名声明中找出应该使用的声明。

3. 对于未限定的名字（Unqualified name），其名字查找的顺序从局部作用域依次往外到全局作用域。在与使用名字的作用域同级的作用域内查找时，只查找使用名字的位置前面的作用域。如果最终没有找到相关声明，则程序是病态的。
4. 在类作用域内的名字，查找顺序与(1)中的顺序相同，但在每一个类作用域内查找时，应先查找其继承链逆序的每一个类的作用域（如果有），再继续往外层作用域查找。
5. `friend`成员函数中的名字查找过程，与普通类成员函数的名字查找过程相同。
6. `static`成员的定义中的名字查找过程，与普通类成员函数的名字查找过程相同。
7. 类名可以用于`::`作用域决议运算符前，形成嵌套名称说明符（nested-name-specifier），此时称为有限定的名字（Qualified name）。如果出现在`::`前的不是类名，则程序是病态的。
8. 有一元作用域运算符`::`的名字在全局作用域查找，即本翻译单元作用域。
7. 重载决议（如果需要）与访问权限检查在名字查找成功后进行。

### 类型

1. 基础类型有13种。其中：
   + 有符号整数类型有4种：`signed char`、`short`、`int`、`long`。
   + 无符号整数类型有4种：`unsigned short`、`unsigned int`、`unsigned int`、`unsigned long`。
   + 浮点类型有2种：`float`和`double`。
   + `char`为字符类型，在NCC编译器中，`char = signed char`。
   + 布尔类型`bool`有2种取值：`true`和`false`。`bool`类型能够被隐式转换为整数类型。
   + 空类型`void`只能被用在表达式语句、逗号表达式操作数、选择表达式`?:`的第二、三操作数、返回类型为`void`的函数的返回语句表达式。`void`类型的表达式用于其他地方的程序为病态。
2. 复合类型有以下几种：
   + 数组，数组的元素类型不能是引用、任何形式的`void`、函数、抽象类，数组的大小（如果指定）需要为非负的整数常量表达式。
   + 函数，包括其参数类型列表与返回类型
   + 指向`void`或对象或函数或类成员（成员变量/成员函数）的指针，其中`void*`被用作指向未知类型的指针，其表现与`char*`相同。
   + 对象或函数或类成员（成员变量/成员函数）的引用
   + 类（class/struct）
   + 枚举
3. 类型可以被`const`修饰，被`const`修饰的类型在初始化后不能被修改。对数组类型的`const`修饰作用在数组元素上，而不是数组本身。

### 标准转换

标准转换为内建类型的隐式转换。共有这些隐式转换：

1. 左值转换为右值
2. 数组转换为指针
3. 函数转换为函数指针
4. 无`const`修饰的指针/引用右值转换为有`const`修饰的指针/引用右值
5. 整形提升（Integer promotions）：如果转换后的整数类型可以表示装换前的整数类型的全部值，则可以隐式转换
6. 浮点提升：`float`转换为`double`
7. 算术类型、枚举、指针可以被隐式转换为`bool`：0、空指针被转换为`false`，其余值被转换为`true`
8. 0可以被隐式转换为空指针（包括成员指针），对象指针可以被隐式转换为`void`指针（保留`const`限定），指向派生类的指针可以被隐式转换为指向基类的指针（B必须是可访问的完整类型，不然程序是病态），基类的成员指针可以被隐式转换为指向派生类的指针（B如果被D私有继承，程序为病态）。

### 重载

对于函数名，名字查找可能会将名字的使用与多个同名函数声明关联，这些函数的集合被称作重载集（Overload set）。

### 语句

1. 对于`if`、`while`、`for`、`switch`中的condition，其表达式最终会被隐式转换为`bool`类型。
2. 函数调用时的参数求值顺序未定义。

### 表达式

1. 每一个表达式要么是左值（lvalue），要么是右值（rvalue）。左值可以被修改（出现在赋值表达式左侧），而右值不能被赋值。
2. 当左值出现在需要右值的地方时，左值被自动转换为右值。
3. 字符串字面量的类型为`const char[N]`，其中`N`为字符串本身的长度加1（字符串字面量末尾自动添加`\0`）。

### 类

1. 有3种可能隐式生成的函数：
   + 当类的非静态成员与基类均是平凡类型，或是能被默认构造的，且类中没有定义构造函数，编译器会自动为该类生成一个默认构造函数。
   + 当类的非静态成员与基类均是平凡类型，或是有复制构造函数的，且类中没有定义复制构造函数，编译器会自动为该类生成一个复制构造函数。
   + 当类的非静态成员与基类均是平凡类型，或是有复制赋值运算符函数的，且类中没有定义复制赋值运算符函数，编译器会自动为该类生成一个复制赋值运算符函数。
2. `friend`修饰符可以在类定义中用于函数声明、函数定义和类声明。
3. `static`修饰的成员为静态成员，静态成员可以用带限定的标识符（qualified-id）访问。在静态成员函数中不能访问`this`指针。只有在全局作用域中（非函数作用域下）声明的类可以有静态成员。
4. `virtual`可以修饰除构造函数外的非静态成员函数。被`virtual`修饰的成员函数是*动态绑定*的，此时该类被称作*多态*类。多态函数可以在声明时添加*纯虚函数*说明（`= 0`）。如果一个类中存在纯虚函数，则该类为抽象类，不能被实例化。
5. 若在派生类中也继承了同名、有相同的参数列表、相同的CV限定符的函数，其被称作*覆盖*了基类的虚函数。该用于覆盖的函数的返回类型需要原函数的返回类型相同，*协变*（covariant）不被支持。每个虚函数都有其*最终覆盖函数*，它是进行虚函数调用时最终绑定的函数。
6. 在类的构造或销毁的过程中调用正在被构造或销毁的类的虚函数时，调用的是当前构造函数或析构函数的类中的最终覆盖函数，而不是其派生类的最终覆盖函数；在类的构造或销毁的过程中调用当前正在被构造或销毁的类的纯虚函数的行为是未定义的。
7. 类的成员初始化顺序与构造函数初始化列表中的顺序无关，其按照成员在类中定义的顺序进行。
8. 非静态成员函数与成员初始化列表中的`this`关键字为一个纯右值表达式，值为调用该函数的对象的地址。

### 程序

1. 程序的起点为名为`main`的函数，该函数不应是重载的。函数的返回值应该声明为`int`。一般而言允许以下两种`main`的定义：
   + `int main() { /*... */ }`
   + `int main(int argc, char** argv) { /*... */ }`
2. 有着静态储存周期的对象在程序开始前被常量表达式初始化或零初始化（若没有指定初始化表达式）。这些对静态储存周期对象的构造顺序与它们在翻译单元中的定义顺序相同。
3. 静态储存周期对象的析构在程序结束后（`main`返回后）执行，其析构顺序与构造顺序相反。