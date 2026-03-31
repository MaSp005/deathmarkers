FROM oven/bun:1-alpine AS base
WORKDIR /usr/src/app

FROM base AS install
RUN mkdir --parents /temp/prod
COPY package.json bun.lock /temp/prod/
RUN cd /temp/prod && bun install --production --frozen-lockfile

FROM base AS release
COPY --from=install /temp/prod/node_modules node_modules
COPY . .

USER bun
EXPOSE 8048/tcp
ENTRYPOINT [ "bun", "run", "start" ]
