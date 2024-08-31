## CMake project setup

```sh
mkdir debug
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug ..

cd ..
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Vite project setup

### Local development environment

Create the configuration file `captainlog-dev.conf` in the debug directory:

```json
{
  "ui-dev-url": "http://localhost:18000"
}
```
