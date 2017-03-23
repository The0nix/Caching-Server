# Проект "Кэширующий веб-сервер".
### Таболов Тамерлан
### Группа БПМИ154-1

 * [Отчёт](#report)
 * [Описание актуальности решаемой задачи](#description)
 * [Используемые технологии](#technologies)
 * [Примерный план](#plan)
 * [Installation and usage](#usage)

# <a id="report"></a> Отчёт
Отчёт к КТ2 можно найти по ссылке: [отчёт](https://yadi.sk/i/m2RZoSbW3GGADV)

# <a id="description"></a> Описание актуальности решаемой задачи
Проект является учебным и потому не претендует на выход в релиз. На данный момент рынок веб-серверов практически полностью окуппирован серверами *Apache* и *nginx*, из которых второй намного ближе к теме проекта, так как так же относится к разряду кэширующих веб-серверов.

# <a id="technologies"></a> Технологии 
В проекте будут использоваться следующие технологии:
 * Технологии стандартной библиотеки языка C, в том числе низкоуровневые технологии для работы с сокетами.
 * Технологии ядра **Linux**.
 * Протокол **HTTP**
 * **Epoll** — для предотвращения блокировки потока во время соединения с клиентом и как гораздо менее ресурсоёмкая альтернатива созданию нескольких процессов.

# <a id="plan"></a> План 

На данный момент реализовано подключение клиентов, парсинг HTTP запросов, выдача файлов по запросам с использованием технологии Epoll и обработка запросов к несуществующим файлам. Планируется так же
 * Реализовать перенаправление запросов.
 * Добавить разделение на потоки.
 * Оформить сервер в виде службы *systemd*.

 Скорее всего не планируется
 * Релизация поддержки WebSocket-прокси

# <a id="usage"></a> Installation and usage 

    $ git clone https://github.com/The0nix/Caching-Server.git
    $ cd Caching-Server
    $ mkdir bin; cd bin
    $ cmake ../
    $ make
    $ ./caser -d -p 8080
    
### Options:
 * `-d` — debug info
 * `-p port` — specify port
 * `-h` — help
