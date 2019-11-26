library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

ppconserwork <- read.csv("evaluation/4-serializing-workers.csv", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
ppconserwork$avg <- rowMeans(ppconserwork[,2:11])
ppconserwork$sdev <- apply(ppconserwork[,2:11], 1, sd)
ppconserwork$proto <- 'deserializing workers'

ppcondeserwork <- read.csv("evaluation/4-deserializing-workers.csv", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
ppcondeserwork$avg <- rowMeans(ppcondeserwork[,2:11])
ppcondeserwork$sdev <- apply(ppcondeserwork[,2:11], 1, sd)
ppcondeserwork$proto <- 'serializing workers'

ppdf <- rbind(ppconserwork)
ppdf <- rbind(ppdf, ppcondeserwork)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev


pp_plot <- ggplot(ppdf, aes(x=num_workers, y=avg, color=proto)) +
  geom_line() + # size=0.8) +
  geom_point(aes(shape=proto), size = 2, stroke=0.8) +
  geom_errorbar(
    mapping=aes(
      ymin=lower,
      ymax=upper
    ),
    width=0.2
  ) +
  scale_shape_manual(values=c(1, 4, 3)) +
  scale_x_continuous(breaks=seq(0, 5, 1)) + # expand=c(0, 0), limits=c(0, 10)
  scale_y_continuous(limits=c(50000000, 120000000), breaks=seq(40000000, 140000000, 10000000)) + # expand=c(0, 0), limits=c(0, 10)
  theme_bw() +
  theme(
    legend.title=element_blank(),
    legend.key=element_rect(fill='white'), 
    legend.background=element_rect(fill="white", colour="black", size=0.25),
    legend.direction="vertical",
    legend.justification=c(0, 1),
    legend.position=c(0.33,0.4),
    # legend box background color
    legend.box.margin=margin(c(3, 3, 3, 3)),
    legend.key.size=unit(0.8, 'lines'),
    text=element_text(size=9),
    strip.text.x=element_blank()
  ) +
  # scale_color_grey() +
  scale_color_brewer(type="qual", palette=6) +
  ggtitle("Four fixed workers") +
  labs(x="workers", y="throughput [msg/s]")

tikz(file="figs/4-fixed-workers.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/4-fixed-workers.pdf", plot=pp_plot, width=3.4, height=2.3)

