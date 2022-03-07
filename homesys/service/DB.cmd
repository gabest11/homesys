@echo off
sqlmetal /namespace:Homesys.Data /context:HomesysDB /code:DB.Data.cs /language:csharp %PROGRAMDATA%\Homesys\homesys.sdf
sqlmetal /namespace:Homesys.Data /dbml:DB.Data.dbml %PROGRAMDATA%\Homesys\homesys.sdf