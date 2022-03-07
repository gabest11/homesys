@echo off
svcutil --language:CS --namespace:*,Homesys.LocalService http://localhost:24935/
del output.config
svcutil --language:CS --namespace:*,Homesys.LocalService http://localhost:24936/
del output.config