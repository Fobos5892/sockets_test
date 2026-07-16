# Modbus TCP Client-Server Chat

C++ клиент и сервер для Linux с протоколом Modbus TCP (MBAP + custom FC `0x41`).

## Сборка

Через скрипт (рекомендуется):

```bash
./build.sh server
./build.sh client
./build.sh all -t Release
./build.sh tests --clean
```

Параметры: `server|client|tests|modbus_tests|modbus_bench|bench|check|asan|all`, `-t Debug|Release` (`Release` = `-O3 -DNDEBUG`), `--no-sanitize`, `-c/--clean`, `-j N`.

Unit-тесты по умолчанию идут под **ASan/UBSan/LSan**. Отключить: `--no-sanitize`.

Полная проверка unit + bench:

```bash
./build.sh check -t Release
```

Или вручную через CMake (таргет обязателен):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target server
cmake --build build --target client
```

Бинарники попадают в `out/<Debug|Release>/`.

## Конфигурация

- [`config/server.conf`](config/server.conf) — `bind_ip`, `port`
- [`config/client.conf`](config/client.conf) — `server_ip`, `port`, `nickname` (опционально)

## Запуск

```bash
./out/Debug/server
./out/Debug/client
./out/Debug/client client2.conf
```

Бинарники автоматически читают конфиги из подкаталога `config/` рядом с собой.
После сборки шаблоны копируются в `out/<Debug|Release>/config/`:

- `server.conf` — настройки сервера (`bind_ip`, `port`)
- `client.conf` — настройки клиента (`server_ip`, `port`, `nickname`)
- `client2.conf` — второй клиент с другим `nickname`

Изменяйте эти файлы перед запуском. Путь к конфигу можно передать аргументом (`client2.conf` или `config/client2.conf`).

## Формат сообщений

В консоли клиента:

```
2:hello
bob:hello
/users
```

- `2:hello` — отправить клиенту с id `2`
- `bob:hello` — отправить клиенту с nickname `bob`
- `/users` — запросить у сервера список подключённых клиентов

При подключении и отключении клиентов все остальные получают уведомления `[presence]`. После регистрации nickname другим клиентам приходит обновление с именем.

Сервер логирует все сообщения. Клиент получает только сообщения, адресованные ему.

## Архитектура (MVVM + SOLID)

```
common/
  Config, Protocol, ModbusTcp, Message
  transport/IMessageTransport, ModbusMessageTransport   — DIP: абстракция транспорта

server/
  model/IClientRegistry, ClientRegistry                 — SRP: состояние клиентов
  transport/IListenSocket, TcpListenSocket              — SRP: listen/accept
  view/IServerView, ServerLogView                       — SRP: только вывод
  viewmodel/
    ServerViewModel                                     — оркестрация event-loop
    MessageDispatcher + IMessageHandler                 — OCP: новый тип = новый handler
    PresenceBroadcaster                                 — SRP: broadcast presence
  Server.* + main.cpp                                   — composition root (DIP)

client/
  model/ClientSession                                   — SRP: состояние сессии
  transport/IClientConnector, TcpClientConnector          — SRP: TCP connect
  view/IClientInputView, IClientOutputView              — ISP: разделённый ввод/вывод
  viewmodel/
    ClientViewModel                                     — оркестрация
    IncomingMessagePresenter                            — SRP: AppMessage → View
    OutgoingMessageBuilder                              — SRP: сборка исходящих payload
    InputCommandParser                                  — SRP: парсинг команд
  main.cpp                                              — composition root (DIP)
```

| Принцип | Применение |
|---------|------------|
| **S** | Каждый класс — одна ответственность (registry, transport, presenter, handler…) |
| **O** | `MessageDispatcher` + `IMessageHandler`: расширение без правки диспетчера |
| **L** | `TcpListenSocket`, `ClientRegistry`, `ConsoleClientView` подставляются через интерфейсы |
| **I** | `IClientInputView` / `IClientOutputView` вместо одного «толстого» view |
| **D** | ViewModel зависит от `IMessageTransport`, `IClientRegistry`, `IListenSocket`, не от реализаций |

- **Model** — данные, протокол, реестр
- **View** — только I/O
- **ViewModel** — бизнес-логика через абстракции

## Протокол

MBAP header + PDU:

- FC: `0x41`
- Типы: Register, AssignId, Chat, Deliver, Error, UserJoined, UserLeft, ListUsers, UsersList

## Тесты

```bash
./build.sh tests                 # ASan/UBSan/LSan по умолчанию
./build.sh tests --no-sanitize   # без санитайзеров
```

Через `./build.sh tests` в конце:
`SANITIZER: OK` и `PASSED: x, FAILED: y, UNAVAILABLE: z`

Покрытие (client + server):

- `Config` — парсинг конфигов и значения по умолчанию
- `ModbusTcp` — encode/decode и socketpair read/write
- `Protocol` — round-trip всех типов сообщений
- `Message` — шаблонный serialize/deserialize и парсинг `recipient:text`
- Client — `parse_input_line`, `OutgoingMessageBuilder`, `IncomingMessagePresenter`
- Server — `ClientRegistry`, `MessageDispatcher` через `MemoryMessageTransport` (без TCP)
- Integration — assign id, доставка по id/nickname, ошибки, presence, список пользователей (реальный TCP)

Общие моки: [`testing/MemoryMessageTransport.hpp`](testing/MemoryMessageTransport.hpp), [`testing/NullViews.hpp`](testing/NullViews.hpp).

## Санитайзеры (утечки / UB)

Включены **по умолчанию** для `./build.sh tests`, `modbus_tests`, `check`, `all` (часть с тестами).

На Linux ASan включает **LeakSanitizer**. Каталог: `build-asan`, артефакты: `out/Debug-asan/`.

```bash
./build.sh tests              # с санитайзерами
./build.sh tests --no-sanitize # без них
```

При падении смотрите `ERROR: AddressSanitizer` / `LeakSanitizer` / `UndefinedBehaviorSanitizer`. CI job `test` тоже гоняет sanitized tests.

## Бенчмарки

Google Benchmark, рекомендуется **Release (`-O3`)**:

```bash
./build.sh bench -t Release
# или вместе с unit-тестами:
./build.sh check -t Release
```

Фильтры и JSON:

```bash
./out/Release/modbus_bench --benchmark_filter=Client|Server|E2E
./out/Release/modbus_bench --benchmark_format=json --benchmark_out=bench.json
```

Что меряется (без сокетов, через memory mock):

- common: `ModbusTcp`, `Protocol`, `Message`
- server: `ClientRegistry`, e2e dispatcher Chat/ListUsers
- client: parser/builder/presenter, e2e send/receive
- round-trip: client → server dispatch → client present

## CI

GitHub Actions workflow: [`.github/workflows/ci.yml`](.github/workflows/ci.yml)

На каждый push/PR:

- job `test`: `server`/`client`/`modbus_bench` + unit-тесты под ASan/UBSan/LSan
- job `bench`: Release (`-O3`) `modbus_bench`, JSON artifact (без fail по latency)