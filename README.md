# Modbus TCP Client-Server Chat

C++ клиент и сервер для Linux с протоколом Modbus TCP (MBAP + custom FC `0x41`).

## Сборка

Через скрипт (рекомендуется):

```bash
./build.sh server
./build.sh client
./build.sh all
./build.sh tests --clean
```

Параметры: `server|client|tests|modbus_tests|modbus_bench|bench|check|asan|all`, `-t Debug|Release` (`Release` = `-O3 -DNDEBUG`), `--no-sanitize`, `-c/--clean`, `-j N`.

`all` собирает `server`/`client`/`modbus_bench` и в **Debug**, и в **Release**, затем гоняет sanitized-тесты.

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
./out/Debug/client client2
```

Из `out/Release` то же самое: `./server`, `./client`, `./client client2`.

Бинарники автоматически читают конфиги из подкаталога `config/` рядом с собой.
После сборки шаблоны копируются в `out/<Debug|Release>/config/`:

- `server.conf` — настройки сервера (`bind_ip`, `port`)
- `client.conf` — настройки клиента (`server_ip`, `port`, `nickname`)
- `client2.conf` — второй клиент с другим `nickname`

Изменяйте эти файлы перед запуском. Аргумент — короткое имя конфига: `client2` (к нему добавится `.conf`). По-прежнему работают `client2.conf`, `config/client2` и абсолютный путь.

## Формат сообщений

В консоли клиента:

```
2:hello
bob:hello
all:hello
/users
/exit
```

- `2:hello` — отправить клиенту с id `2`
- `bob:hello` — отправить клиенту с nickname `bob`
- `all:hello` — отправить всем подключённым клиентам (кроме себя); nickname `all` зарезервирован
- `/users` — запросить у сервера список подключённых клиентов
- `/key 2` — отправить свой публичный ключ клиенту с id `2` (обычно уходит автоматически при join)
- `/exit` (или `exit`) — выйти из клиента; при любом закрытии (команда, Ctrl+C, закрытие консоли) соединение закрывается и остальные получают `[presence] User left`
- при остановке сервера (Ctrl+C / закрытие) все клиенты получают обрыв соединения и сами завершаются

### E2E шифрование

Сообщения между клиентами шифруются end-to-end (X25519 + XChaCha20-Poly1305 через [Monocypher](https://monocypher.org/), vendored в `common/crypto/third_party/`). Сервер — blind relay: в лог пишется только `Chat from id=… to …` без тела.

- **Identity keypair** — у каждого клиента; peer-ключи через `KeyOffer` / `PeerKey`
- **Room key** — создаёт первый клиент в комнате (`AssignId.create_room`); дальше gossip через `RoomKeyOffer` только к пирам с известным pubkey
- **Local keystore** — при выходе шифруется на диске (`<nickname>.keys` или `keystore_path` / `keystore_password` в client.conf)
- **Client log** — подробности в файл (`<nickname>.log` или `log_path` в client.conf); на экран — краткие ошибки/notice
- **Broadcast `all:`** — симметрично room key

Threat model (resume demo): сервер и сеть не видят plaintext; скомпрометированный клиент и утечка пароля keystore — вне модели.

### Ban (консоль сервера)

На stdin сервера только admin-команды:

```
ban:1,2,3
```

Забаненные id не шлют и не получают сообщения, сокет disconnect, id не переиспользуется до рестарта сервера. Остальным клиентам уходит `BannedIds` — они забывают pubkey этих id.

Освободившиеся id после disconnect переиспользуются (берётся наименьший свободный), кроме забаненных.

При подключении и отключении клиентов все остальные получают уведомления `[presence]`. После регистрации nickname другим клиентам приходит обновление с именем.

## Архитектура (MVVM + SOLID)

```
common/
  Config, Protocol, ModbusTcp, Message
  crypto/IMessageCipher, MonocypherMessageCipher, PeerKeyStore, EncryptedKeyStoreFile
  transport/IMessageTransport, ModbusMessageTransport   — DIP: абстракция транспорта
  concurrency/ICommand, QueuedCommandExecutor           — общая Command-очередь

server/
  model/IClientRegistry, ClientRegistry                 — SRP: состояние клиентов
  transport/IListenSocket, TcpListenSocket              — SRP: listen/accept
  view/IServerView, ServerLogView                       — SRP: только вывод
  viewmodel/
    ServerViewModel                                     — оркестрация event-loop + ban-list
    AdminCommandParser                                  — admin console (`ban:`)
    MessageDispatcher + IMessageHandler                 — OCP: новый тип = новый handler
    PresenceBroadcaster                                 — SRP: broadcast presence
  Server.* + main.cpp                                   — composition root (DIP)

client/
  model/ClientSession                                   — SRP: состояние сессии
  transport/IClientConnector, TcpClientConnector          — SRP: TCP connect
  view/IClientInputView, IClientOutputView              — ISP: разделённый ввод/вывод
  view/QueuedClientOutputView + ClientOutputCommands    — адаптер view → QueuedCommandExecutor
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
- Типы: Register, AssignId (`create_room`), Chat, Deliver, Error, UserJoined, UserLeft, ListUsers, UsersList, KeyOffer, RoomKeyOffer, BannedIds, PeerKey, RoomKey
- Chat/Deliver body — opaque (ciphertext для E2E)

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
- Client — `parse_input_line`, `OutgoingMessageBuilder`, `IncomingMessagePresenter`, crypto/keystore
- Server — `ClientRegistry`, `MessageDispatcher`, `AdminCommandParser`
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