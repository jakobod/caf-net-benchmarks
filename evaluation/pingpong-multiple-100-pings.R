library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

pingpong_io <- read.csv("evaluation/data/pingpong-io-100-pings.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_io$avg <- rowMeans(pingpong_io[,2:11])
pingpong_io$sdev <- apply(pingpong_io[,2:11], 1, sd)
pingpong_io$proto <- 'libcaf_io'

pingpong_net <- read.csv("evaluation/data/pingpong-net-100-pings.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_net$avg <- rowMeans(pingpong_net[,2:11])
pingpong_net$sdev <- apply(pingpong_net[,2:11], 1, sd)
pingpong_net$proto <- 'libcaf_net'

pingpong_net_vector <- read.csv("evaluation/data/pingpong-net-100-pings-vector.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_net_vector$avg <- rowMeans(pingpong_net_vector[,2:11])
pingpong_net_vector$sdev <- apply(pingpong_net_vector[,2:11], 1, sd)
pingpong_net_vector$proto <- 'libcaf_net - vector'

ppdf <- rbind(pingpong_io, pingpong_net, pingpong_net_vector)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev


pp_plot <- ggplot(ppdf, aes(x=num_pings, y=avg, color=proto)) +
  geom_line() + # size=0.8) +
  geom_point(aes(shape=proto), size = 1, stroke=0.8) +
  geom_errorbar(
    mapping=aes(
      ymin=lower,
      ymax=upper
    ),
    width=0.2
  ) +
  scale_x_continuous(breaks=seq(1, 64, 2)) + # expand=c(0, 0), limits=c(0, 10)
  scale_y_continuous(labels = human_numbers, limits=c(30000,300000), breaks=seq(30000, 300000, 10000)) + # expand=c(0, 0), limits=c(0, 10)
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
  scale_fill_brewer(palette="Dark2") +
  ggtitle("Pingpong multiple remote nodes 100 Pings") +
  labs(x="remote nodes [#]", y="throughput [pongs/s]")

# tikz(file="figs/pingpong-multiple-100.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/pingpong-100-pings.pdf", plot=pp_plot, width=8, height=5)

