library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

pingpong_net_master <- read.csv("evaluation/data/pingpong-waiting-net-master.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_net_master$avg <- rowMeans(pingpong_net_master[,2:11])
pingpong_net_master$sdev <- apply(pingpong_net_master[,2:11], 1, sd)
pingpong_net_master$proto <- '1 net - master'

pingpong_net_actor_proxy <- read.csv("evaluation/data/pingpong-waiting-net-actor-proxy.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_net_actor_proxy$avg <- rowMeans(pingpong_net_actor_proxy[,2:11])
pingpong_net_actor_proxy$sdev <- apply(pingpong_net_actor_proxy[,2:11], 1, sd)
pingpong_net_actor_proxy$proto <- '2 net - actor-proxy'

pingpong_net_workers <- read.csv("evaluation/data/pingpong-waiting-net-workers.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_net_workers$avg <- rowMeans(pingpong_net_workers[,2:11])
pingpong_net_workers$sdev <- apply(pingpong_net_workers[,2:11], 1, sd)
pingpong_net_workers$proto <- '3 net - workers'

pingpong_io <- read.csv("evaluation/data/pingpong-waiting-io.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_io$avg <- rowMeans(pingpong_io[,2:11])
pingpong_io$sdev <- apply(pingpong_io[,2:11], 1, sd)
pingpong_io$proto <- '4 io'

ppdf <- rbind(pingpong_net_master)
ppdf <- rbind(ppdf, pingpong_net_actor_proxy)
ppdf <- rbind(ppdf, pingpong_net_workers)
ppdf <- rbind(ppdf, pingpong_io)
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
    width=0.2
  ) +
  scale_x_log10(breaks = trans_breaks("log10", function(x) 10^x), labels = trans_format("log10", math_format(10^.x))) +
  scale_y_continuous(labels = human_numbers, limits=c(-5000,190000), breaks=seq(-5000, 190000, 25000)) + # expand=c(0, 0), limits=c(0, 10)
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
  ggtitle("Pingpong waiting for all messages") +
  labs(x="simultaneous ping_messages [#]", y="throughput [pongs/s]")

tikz(file="figs/pingpong.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/pingpong-waiting-with-io.pdf", plot=pp_plot, width=5, height=3)

