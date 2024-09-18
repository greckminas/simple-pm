# simple-pm

Simple Process Manager for Windows

## Usage

You need to write the configuration somewhere and run this command in commandline:

```sh
simple-pm.exe /path/to/config.yaml
```

## Configuration File

The configuration use YAML format (https://yaml.org/). There are two types to run a process, by command or by path with argument(s). Here are some examples to run process:

```yaml
exe-process:
  path: .\program.exe
  checking-interval: 10

exe-with-args:
  path: .\program2.exe
  args: write some args here
  working-dir: C:\Windows
  checking-interval: 5

npm-script:
  command: npm run start
  working-dir: C:\Users\user
  checking-interval: 5
```

Those 3 processes will be monitored by each process' interval. For example, a process named `exe-process` is to run `program.exe` and will re-run every 10 seconds if the current `exe-process` is closed.
