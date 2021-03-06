# Проект "Кэширующий веб-сервер".
### Таболов Тамерлан
### Группа БПМИ154-1

 * [Отчёт](#report)
 * [Описание актуальности решаемой задачи](#description)
 * [Используемые технологии](#technologies)
 * [Примерный план](#plan)
 * [Installation and usage](#usage)

# <a id="report"></a> Отчёт
Отчёт к КТ2 можно найти по ссылке: [отчёт](https://yadi.sk/i/jfhp_2UV3GGAS5)

# <a id="description"></a> Описание актуальности решаемой задачи
Проект является учебным и потому не претендует на выход в релиз. На данный момент рынок веб-серверов практически полностью окуппирован серверами *Apache* и *nginx*, из которых второй намного ближе к теме проекта, так как так же относится к разряду кэширующих веб-серверов.

# <a id="technologies"></a> Технологии 
В проекте будут использоваться следующие технологии:
 * Технологии стандартной библиотеки языка C, в том числе низкоуровневые технологии для работы с сокетами.
 * Технологии ядра **Linux**.
 * Протокол **HTTP**
 * **Epoll** — для предотвращения блокировки потока во время соединения с клиентом и как гораздо менее ресурсоёмкая альтернатива созданию нескольких процессов.

# <a id="plan"></a> Features

На данный момент реализовано подключение клиентов, парсинг HTTP запросов, выдача файлов по запросам с использованием технологии Epoll, обработка ошибочных запросов и запросов к несуществующим файлам, проксирование.

# <a id="usage"></a> Installation and usage 

    $ git clone https://github.com/The0nix/Caching-Server.git
    $ cd Caching-Server
    $ mkdir bin; cd bin
    $ cmake ../
    $ make
    $ ./caser -p 8080  # launch in default mode
    $ ./caser -p 8080 -a http://pikabu.ru  # launch in proxy mode
    
В папку bin необходимо поместить файлы сайта. Например: `echo ‘<html><body><h1>Hello, world</h1></body></html>’ > index.html`.

Для доступа к серверу можно воспользоваться браузером, обратившись к нужному адресу (по умолчанию: `localhost:<port>/<path>`) или, например, утилитой *curl*: `curl localhost:<port>/<path>`.

Для отключения сервера используется комбинация клавиш `<Ctrl+C>`
    
### Options:
 * `-d` — debug info
 * `-p port` — specify port
 * `-h` — help
 * `-a service://host` — proxy to `host` via `service`
