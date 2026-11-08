Компиляция:

    gcc -std=c11 -Wall -Wextra main.c csv_parser.c -o csvstat

Запуск:

    ./csvstat --help
    
    ./csvstat schema <file.csv>
    
    ./csvstat stats  <file.csv>
    
