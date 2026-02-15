# Etapa 1: Construirea (Build)
FROM eclipse-temurin:21-jdk-jammy AS build
COPY . .
# Rulăm Maven pentru a crea fișierul .jar (sărim peste teste pentru viteză)
RUN ./mvnw clean package -DskipTests

# Etapa 2: Rularea (Run)
FROM eclipse-temurin:21-jre-jammy
# Copiem doar executabilul din prima etapă
COPY --from=build /target/*.jar app.jar
# Expunem portul pe care ascultă Spring Boot
EXPOSE 8081
# Comanda de pornire
ENTRYPOINT ["java", "-jar", "/app.jar"]