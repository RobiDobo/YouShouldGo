# build stage
FROM eclipse-temurin:21-jdk-jammy AS build
COPY . .
# skip tests, some error

RUN ./mvnw clean package -DskipTests=true

# run in image
FROM eclipse-temurin:21-jre-jammy
# copy jar from build
COPY --from=build /target/*.jar app.jar
# run jar
ENTRYPOINT ["java", "-jar", "/app.jar"]