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

Параметры: `server|client|tests|modbus_tests|all`, `-t Debug|Release`, `-c/--clean`, `-j N`.

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
./out/Debug/server config/server.conf
./out/Debug/client config/client.conf
```

Для второго клиента создайте отдельный конфиг с другим `nickname`.

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
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target modbus_tests
ctest --test-dir build --output-on-failure
```

Покрытие:

- `Config` — парсинг конфигов и значения по умолчанию
- `ModbusTcp` — encode/decode и socketpair read/write
- `Protocol` — round-trip всех типов сообщений
- `Message` — шаблонный serialize/deserialize и парсинг `recipient:text`
- Integration — assign id, доставка по id/nickname, ошибки, presence, список пользователей

## CI

GitHub Actions workflow: [`.github/workflows/ci.yml`](.github/workflows/ci.yml)

На каждый push/PR: configure, сборка `server`, `client`, `modbus_tests`, запуск `ctest`.
