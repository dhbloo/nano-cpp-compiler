# Nano-cpp 语言规范

Nano-cpp是C++的一个语言子集，该子集涵盖了C++的部分基本特性，其部分参考了[Embedded C++](https://en.wikipedia.org/wiki/Embedded_C%2B%2B)的语言设计选择。相比C++03标准，特别去除了以下的语言特性，以简化编译器的实现：

+ 异常处理
+ 多重继承
+ 虚基类
+ 命名空间
+ 模版
+ RTTI
+ C++ Style Cast（`static_cast`、`dynamic_cast`、`reinterpret_cast`、`const_cast`）
+ 联合体（Union）
+ 储存类型修饰符（`auto`、`register`、`static`、`extern`、`mutable`）
+ CV修饰符中的`volatile`
+ 函数修饰符中的`explicit`
+ `goto`语句
+ 部分运算符重载（`new`、`delete`、`new[]`、`delete[]`）
+ 位域
+ 可变参数

特别的，为了进一步简化实现，仅支持单文件编译，因此以下语言特性也一并去除：

+ 头文件引用
+ 预处理宏
+ 链接规范说明（如`extern "C"`）
+ `extern`外部链接修饰符
+ `inline`内联修饰符

总体而言，语言保留的特性主要为C与C++共有的部分，加上C++的面向对象语法与一些C++常见的语言特性（如引用）。

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

整数字符串只支持10进制与16进制：

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
register		return			short			signed			sizeof
static			struct			switch			this			true
typedef			unsigned		virtual			void			while
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
    '::' operator-function-id
    
nested-name-specifier:
	class-name '::'
	nested-name-specifier class-name '::'

postfix-expression:
    primary-expression
    postfix-expression '[' expression ]
    postfix-expression '(' [expression-list] ')'
    simple-type-specifier '(' [expression-list] ')'
    postfix-expression '.' id-expression
    postfix-expression '->' id-expression
    postfix-expression '.' pseudo-destructor-name
    postfix-expression '->' pseudo-destructor-name
    postfix-expression '++'
    postfix-expression '--'
    
expression-list:
    assignment-expression
    expression-list ',' assignment-expression

pseudo-destructor-name:
    ['::'] [nested-name-specifier] [type-name '::'] '~' type-name

unary-expression:
    postfix-expression
    '++' cast-expression
    '--' cast-expression
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
    
new-expression:
	['::'] 'new' [new-placement] new-type-id [new-initializer]
	['::'] 'new' [new-placement] '(' type-id ')' [new-initializer]

new-placement:
	'(' expression-list ')'

new-type-id:
    type-specifier-seq [new-declarator]

new-declarator:
	ptr-operator [new-declarator]
    direct-new-declarator

direct-new-declarator:
    '[' expression ']' 
    direct-new-declarator '[' constant-expression ']'
    
new-initializer:
	'(' [expression-list] ')'

delete-expression:
	['::'] 'delete' cast-expression
	['::'] 'delete' '[' ']' cast-expression

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
    type-specifier-seq declarator '=' assignment-expression
    
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
	[decl-specifier-seq] [init-declarator-list] ';'
	
decl-specifier:
    type-specifier
    function-specifier
    'friend'
    'typedef'
    
decl-specifier-seq:
	[decl-specifier-seq] decl-specifier
	
function-specifier:
    'virtual'

type-specifier:
    simple-type-specifier
    class-specifier
    enum-specifier
    elaborated-type-specifier
    cv-qualifier
    
simple-type-specifier:
    ['::'] [nested-name-specifier] type-name
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
    
elaborated-type-specifier:
    class-key ['::'] [nested-name-specifier] identifier
    'enum' ['::'] [nested-name-specifier] identifier
    
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
    ptr-operator declarator

direct-declarator:
    declarator-id
    direct-declarator '(' [parameter-declaration-list] ')' [cv-qualifier]
    direct-declarator '[' [constant-expression] ']' '(' declarator ')'

ptr-operator:
    '*' [cv-qualifier-seq]
    '&'
    ['::'] nested-name-specifier '*' [cv-qualifier]
    
cv-qualifier-seq:
	cv-qualifier [cv-qualifier-seq]
    
cv-qualifier:
	'const'

declarator-id:
    id-expression
    ['::'] [nested-name-specifier] type-name

type-id:
	type-specifier-seq [abstract-declarator]

type-specifier-seq:
	type-specifier [type-specifier-seq]
	
abstract-declarator:
    ptr-operator [abstract-declarator]
    direct-abstract-declarator

direct-abstract-declarator:
	[direct-abstract-declarator] '(' [parameter-declaration-list] ')' [cv-qualifier]
	[direct-abstract-declarator] '[' [constant-expression] ']' '(' abstract-declarator ')'

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
    class-key [identifier] [base-clause]
    class-key nested-name-specifier identifier [base-clause]

class-key:
    'class'
    'struct'
    
member-specification:
    member-declaration [member-specification]
    access-specifier ':' [member-specification]

member-declaration:
    [decl-specifier-seq] [member-declarator-list] ';'
    function-definition [';']
    ['::'] nested-name-specifier unqualified-id ';'

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
    ['::'] [nested-name-specifier] class-name
    access-specifier ['::'] [nested-name-specifier] class-name

access-specifier:
    'private'
    'protected'
    'public'
```

### 9. 特殊成员函数

```
conversion-function-id:
	operator conversion-type-id

conversion-type-id:
	type-specifier-seq [conversion-declarator]

conversion-declarator:
	ptr-operator [conversion-declarator]

ctor-initializer:
	':' mem-initializer-list

mem-initializer-list:
    mem-initializer
    mem-initializer ',' mem-initializer-list

mem-initializer:
	mem-initializer-id '(' [expression-list] ')'

mem-initializer-id:
	['::'] [nested-name-specifier] class-name
	identifier
```

### 10. 重载

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
    '()' 
    '[]'
```





