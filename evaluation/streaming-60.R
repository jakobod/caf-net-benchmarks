library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

streaming_net <- read.csv("evaluation/data/streaming-net-60.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
streaming_net$avg <- rowMeans(streaming_net[,2:60])
streaming_net$sdev <- apply(streaming_net[,2:60], 1, sd)
streaming_net$proto <- 'libcaf_net'

streaming_io <- read.csv("evaluation/data/streaming-io-60.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
streaming_io$avg <- rowMeans(streaming_io[,2:60])
streaming_io$sdev <- apply(streaming_io[,2:60], 1, sd)
streaming_io$proto <- 'libcaf_io'

ppdf <- rbind(streaming_net, streaming_io)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

pp_plot <- ggplot(ppdf, aes(x=num_pings, y=avg, color=proto)) +
  geom_line() + # size=0.8) +
  geom_point(aes(shape=proto), size = 2, stroke=0.8) +
  geom_errorbar(
    mapping=aes(
      ymin=lower,
      ymax=upper
    ),
    width=0.4
  ) +
  scale_x_continuous(breaks=seq(1, 64, 2)) + # expand=c(0, 0), limits=c(0, 10)
  scale_y_continuous(labels = human_numbers, limits=c(10000000,22000000), breaks=seq(10000000, 22000000, 1000000)) + # expand=c(0, 0), limits=c(0, 10)
  theme_bw() +
  theme(
    legend.title=element_blank(),
    legend.key=element_rect(fill='white'), 
    legend.background=element_rect(fill="white", colour="black", size=0.25),
    legend.direction="vertical",
    legend.justification=c(0, 1),
    legend.position=c(0,1),
    legend.box.margin=margin(c(3, 3, 3, 3)),
    legend.key.height=unit(0.4,"line"),
    legend.key.size=unit(0.6, 'lines'),
    text=element_text(size=9),
    strip.text.x=element_blank()
  ) +
  scale_color_brewer(type="qual", palette=6) +
  scale_fill_brewer(palette="Dark2") +
  ggtitle("Streaming") +
  labs(x="remote nodes [#]", y="throughput [messages/s]")

# tikz(file="figs/streaming.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/streaming-60.pdf", plot=pp_plot, width=8, height=5)

